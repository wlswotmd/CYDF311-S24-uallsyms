#ifndef _UALLSYMS_UALLSYMS_H
#define _UALLSYMS_UALLSYMS_H

#include <stddef.h>

#include <uallsyms/uallsyms.h>

struct uas {
    size_t kernel_base_address;
    uas_aar_t aar_function;
};

#endif /* _UALLSYMS_UALLSYMS_H */