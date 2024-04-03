#ifndef _LIBUALLSYMS_H
#define _LIBUALLSYMS_H

#include <uallsyms/types.h>

extern uas_t *uas_init(uas_aar_t aar_func);
extern uas_t *uas_init2(uas_aar_t aar_func, arch_t arch);

#endif /* _LIBUALLSYMS_H */