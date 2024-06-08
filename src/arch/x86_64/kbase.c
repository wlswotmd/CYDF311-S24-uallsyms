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

kaddr_t x86_64_resolve_div_by_0_handler(uas_t *uas)
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

