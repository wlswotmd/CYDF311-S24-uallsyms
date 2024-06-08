#include <uallsyms/types.h>

#define PAGE_SIZE 0x1000
#define UNIQUE_SEQS "\x30\x00\x31\x00\x32\x00\x33\x00\x34\x00\x35\x00\x36\x00\x37\x00\x38\x00\x39\x00"

#define ALIGN(x, align)   (((x) + (align) - 1) & ~((align) - 1))

kaddr_t kallsyms_lookup_name(uas_t *uas, const char *name);