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

void *my_malloc(size_t size);
void my_free(void * ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);

void *add_chunk(size_t alloc_size);
void split_chunk(hdr *chk, size_t alloc_size);
int increase_heap(size_t size);
int find_scale(int num);

void create_hdr(hdr *chk_hdr, hdr *next, hdr *prev, size_t alloc_size);
uintptr_t align16(uintptr_t addr);
hdr *init_heap(void);
void print_heap(void);

#endif