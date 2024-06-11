#ifndef _LIBUALLSYMS_H
#define _LIBUALLSYMS_H

#include "types.h"

extern kaddr_t uas_resolve_pop_rdi(uas_t *uas);
extern kaddr_t uas_resolve_kpti_trampoline(uas_t *uas);
extern u64 uas_resolve_canary(uas_t *uas);
extern int uas_aar(uas_t *uas, void *to, kaddr_t from, size_t n);
extern kaddr_t uas_lookup_name(uas_t *uas, const char *name);
extern uas_t *uas_init(uas_aar_t aar_func);
extern uas_t *uas_init2(uas_aar_t aar_func, arch_t arch);

#endif /* _LIBUALLSYMS_H */