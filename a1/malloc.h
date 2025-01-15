#ifndef MALLOC_H
#define MALLOC_H

#include <stdio.h>
#include <stdint.h>

#define HEAP_INCR  64 * 1024
#define HEAP_BASE  64

typedef struct hdr{
    int free;
    size_t size;
    struct hdr *next_chk;
    struct hdr *prev_chk;
} hdr;

void *malloc(size_t size);
void *add_chunk(size_t alloc_size);
void split_chunk(hdr *chk, size_t alloc_size);
void increase_heap();

void create_hdr(hdr *chk_hdr, hdr *next, hdr *prev, size_t alloc_size);
uintptr_t align16(uintptr_t addr);
uintptr_t init_heap(void);

#endif