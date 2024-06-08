#ifndef _LIBUALLSYMS_H
#define _LIBUALLSYMS_H

#include <libuallsyms/types.h>

extern kaddr_t uas_lookup_name(uas_t *uas, const char *name);
extern uas_t *uas_init(uas_aar_t aar_func);
extern uas_t *uas_init2(uas_aar_t aar_func, arch_t arch);

#endif /* _LIBUALLSYMS_H */