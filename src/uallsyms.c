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

// kaddr_t uas_resolve_kernel_base(uas_t *uas)
// {
//     if (uas->kbase != UNKNOWN_KADDR)
//         return uas->kbase;

//     switch (uas->arch) {
//     case X86_64:
//         uas->kbase = x86_64_resolve_kernel_base(uas);
//         return uas->kbase;
//     default: /* Not implemented */
//         return UNKNOWN_KADDR;
//     }
// }

kaddr_t uas_lookup_name(uas_t *uas, const char *name)
{
    return kallsyms_lookup_name(uas, name);
}

uas_t *uas_init2(uas_aar_t aar_func, arch_t arch)
{
    uas_t *uas;

    uas = calloc(1, sizeof(*uas));
    if (!uas)
        return NULL;

    uas->aar_func = aar_func;
    uas->kbase = UNKNOWN_KADDR;
    uas->arch = arch;
    /* architecture와는 다르게 kernel version은 굳이 사용자가 지정하게 할 필요는 없을 듯 하다. */
    uas->kver = current_kver(); 

    uas->kallsyms_cache.initialized = false;
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