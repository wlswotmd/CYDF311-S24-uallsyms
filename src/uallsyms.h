#ifndef _UALLSYMS_UALLSYMS_H
#define _UALLSYMS_UALLSYMS_H

#include <stddef.h>

#include <uallsyms/uallsyms.h>

struct uas {
    kaddr_t kbase;
    kaddr_t kallsyms_token_table;
    uas_aar_t aar_func;
    arch_t arch;
    kver_t kver;
};

#endif /* _UALLSYMS_UALLSYMS_H */