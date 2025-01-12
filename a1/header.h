#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdint.h>

#define HEAP_CHUNKS  64 * 1024

typedef struct hdr{
    int free;
    size_t size;
    struct hdr *next_chk;
    struct hdr *prev_chk;
} hdr;

void create_hdr(hdr *chk_hdr, hdr *next, hdr *prev, size_t alloc_size);
uintptr_t align16(uintptr_t addr);
uintptr_t init_heap(void);

#endif