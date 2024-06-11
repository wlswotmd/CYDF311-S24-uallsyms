/*
 * uallsyms.c: uallsyms helpers
 *
 * uallsyms library 관련 helper들이 구현되어 있다.
 */

#include <stdlib.h>

#include <uallsyms/types.h>
#include <uallsyms/uallsyms.h>

#include "kinfo/uname.h"
#include "arch/x86_64/kbase.h"
#include "kallsyms/core.h"
#include "utils/log.h"
#include "resolver/canary.h"
#include "resolver/gadget.h"

kaddr_t uas_resolve_pop_rdi(uas_t *uas)
{
    return resolve_pop_rdi(uas);
}

kaddr_t uas_resolve_kpti_trampoline(uas_t *uas)
{
    return resolve_kpti_trampoline(uas);
}

u64 uas_resolve_canary(uas_t *uas)
{
    return resolve_canary(uas);
}

int uas_aar(uas_t *uas, void *to, kaddr_t from, size_t n) 
{
    int ret;

    ret = uas->aar_func(to, from, n);
    if (ret < 0)
        pr_err("[uas_aar] ret < 0\n");

    return ret;
}

kaddr_t uas_lookup_name(uas_t *uas, const char *name)
{
    return kallsyms_lookup_name(uas, name);
}

uas_t *uas_init2(uas_aar_t aar_func, arch_t arch)
{
    uas_t *uas;
    /* ref: https://github.com/torvalds/linux/commit/73bbb94466fd3f8b313eeb0b0467314a262dddb3 */
    DEFINE_KVER(may_use_big_symbol_kver, 6, 1, 0);

    uas = calloc(1, sizeof(*uas));
    if (!uas)
        return NULL;

    uas->aar_func = aar_func;
    uas->kbase = UNKNOWN_KADDR;
    uas->arch = arch;
    /* architecture와는 다르게 kernel version은 굳이 사용자가 지정하게 할 필요는 없을 듯 하다. */
    uas->kver = current_kver(); 

    uas->kallsyms_cache.initialized = false;
    if (kver_ge(uas->kver, may_use_big_symbol_kver))
        uas->kallsyms_cache.may_use_big_symbol = true;
    else
        uas->kallsyms_cache.may_use_big_symbol = false;

    uas->kallsyms_cache.kallsyms_num_syms = 0;
    uas->kallsyms_cache.kallsyms_names = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_markers = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_token_table = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_token_index = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_addresses = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_offsets = UNKNOWN_KADDR;
    uas->kallsyms_cache.kallsyms_relative_base = UNKNOWN_KADDR;

    return uas;
}

uas_t *uas_init(uas_aar_t aar_func)
{
    return uas_init2(aar_func, current_arch());
}