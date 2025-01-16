#ifndef MALLOC_H
#define MALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pp.h>

#define HEAP_INCR   65536   // 64 * 1024
#define NO_MEMORY   -1
#define HEAP_CREATED    1
#define HEAP_INCREASED   2
#define HEADER_SIZE sizeof(header)

typedef struct header {
    struct header *next;
    struct header *prev;
    size_t size;
    bool is_free;
} header;

void *malloc(size_t size);
void *realloc(void* ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void* ptr);

int align16(int val);
void print_heap(void);

#endif