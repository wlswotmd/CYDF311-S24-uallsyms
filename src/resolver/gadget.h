#include <uallsyms/uallsyms.h>

#define POP_RDI_SEQS "\x5f\xc3" /* pop rdi; ret; */
#define KPTI_TRAMPOLINE_SEQS "\x48\x89\xe7" /* mov rdi, rsp; */

kaddr_t resolve_pop_rdi(uas_t *uas);
kaddr_t resolve_kpti_trampoline(uas_t *uas);
