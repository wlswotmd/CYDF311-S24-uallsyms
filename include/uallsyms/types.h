#ifndef _LIBUALLSYMS_TYPES_H
#define _LIBUALLSYMS_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef size_t kaddr_t;
typedef size_t koffs_t;
#define UNKNOWN_KADDR ((kaddr_t)-1)

/* 
 * x64_64 kernel에서 x86 binary를 돌릴 수도 있기 때문에 __amd64__등과 같은 macro를 
 * 이용하기 보다는 uname 등을 이용해 현재 kernel의 architecture를 특정한다.
 */
typedef enum {
    X86,
    X86_64,
    ARM,
    ARM64,
    UNKNOWN_ARCH,
} arch_t;

typedef struct kver {
    uint16_t version;
    uint16_t patch_level;
    uint16_t sub_level;
} kver_t;

#define INVALID_VERSION ((uint16_t)-1)

#define DEFINE_KVER(name, version_, patch_level_, sub_level_) \
    kver_t name = { \
        .version = version_, \
        .patch_level = patch_level_, \
        .sub_level = sub_level_, \
    }

#define DEFINE_INVALID_KVER(name) DEFINE_KVER(name, INVALID_VERSION, INVALID_VERSION, INVALID_VERSION)

inline int is_vaild_kver(kver_t kver)
{
    /*
     * kver.version 이외의 것들은 없을 수도 있지만, 
     * default로 kver.version, kver.patch_level, kver.sub_level은 설정되어 있기 때문에
     * 위 3개를 모두 확인한다.
     * 
     * https://elixir.bootlin.com/linux/v6.8.1/source/init/version-timestamp.c#L15 
     * https://elixir.bootlin.com/linux/v6.8.1/source/Makefile#L1235
     * https://elixir.bootlin.com/linux/v6.8.1/source/Makefile#L367
     * https://elixir.bootlin.com/linux/v6.8.1/source/Makefile#L2
     */

    return (
        kver.version != INVALID_VERSION &&
        kver.patch_level != INVALID_VERSION &&
        kver.sub_level != INVALID_VERSION
    );
}

inline int kver_gt(kver_t lhs, kver_t rhs)
{
    if (lhs.version > rhs.version)
        return 1;
    
    if (lhs.version == rhs.version && lhs.patch_level > rhs.patch_level)
        return 1;

    if (lhs.patch_level == rhs.patch_level && lhs.sub_level > rhs.sub_level)
        return 1;

    return 0;
}

inline int kver_ge(kver_t lhs, kver_t rhs)
{
    if (kver_gt(lhs, rhs) || lhs.sub_level == rhs.sub_level)
        return 1;
    
    return 0;
}

inline int kver_lt(kver_t lhs, kver_t rhs)
{
    return !kver_ge(lhs, rhs);
}

inline int kver_le(kver_t lhs, kver_t rhs)
{
    return !kver_gt(lhs, rhs);
}

typedef int (*uas_aar_t)(void *to, kaddr_t from, size_t n);

/*
 * v6.4 ~
 * (1) kallsyms_num_syms
 * (2) kallsyms_names
 * (3) kallsyms_markers
 * (4) kallsyms_token_table
 * (5) kallsyms_token_index
 * (6) kallsyms_addresses / kallsyms_offsets
 * (7) kallsyms_relative_base
 * (8) kallsyms_seq_of_names
 * 
 * ref: https://github.com/torvalds/linux/commit/404bad70fcf7cb1a36198581e6904637f3c36846
 */
struct kallsyms_cache {
    bool initialized;
    uint32_t kallsyms_num_syms;
    kaddr_t kallsyms_names;
    kaddr_t kallsyms_markers;
    kaddr_t kallsyms_token_table;
    kaddr_t kallsyms_token_index;
    kaddr_t kallsyms_addresses;
    kaddr_t kallsyms_offsets;
    kaddr_t kallsyms_relative_base;
    /* kaddr_t kallsyms_seq_of_names; useless member variable */
};

struct uas {
    kaddr_t kbase;
    uas_aar_t aar_func;
    arch_t arch;
    kver_t kver;
    struct kallsyms_cache kallsyms_cache;
};

typedef struct uas uas_t;

#endif /* _LIBUALLSYMS_TYPES_H */