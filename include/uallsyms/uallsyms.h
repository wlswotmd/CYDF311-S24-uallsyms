#ifndef _LIBUALLSYMS_H
#define _LIBUALLSYMS_H

#include <stddef.h>

struct uas;

typedef struct uas uas_t;
typedef int (*uas_aar_t)(void *to, void *from, size_t n);

extern uas_t *uas_init(uas_aar_t aar_function);

#endif /* _LIBUALLSYMS_H */