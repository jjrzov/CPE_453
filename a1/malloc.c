#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define BUFFER_SIZE 1500

uintptr_t flr = NULL;
uintptr_t tail = NULL;

void *realloc(void *ptr, size_t size);
void free(void * ptr);
void *calloc(size_t nmemb, size_t size);

char buffer[BUFFER_SIZE];

void *malloc(size_t size){
    if (size == 0){
        return NULL;
    }
    if ((hdr *) flr == NULL){
        char *s = "INITIALIZING HEAP";
        snprintf(buffer, BUFFER_SIZE, "%s\n", s);
        // write(2, buffer, BUFFER_SIZE);
        flr = init_heap();  // Need to initialize heap
        tail = flr; // Update tail
    }
    return (void *) add_chunk(size);

}

void *add_chunk(size_t alloc_size){
    hdr *curr_chk = (hdr *) flr;
    // hdr *prev_chk = NULL;
    while (curr_chk != NULL){
        if (curr_chk->free == 0){   // Current chunk is free
            if (curr_chk->size > alloc_size){
                // Able to add to chunk
                uintptr_t uint_chk = (uintptr_t) curr_chk;
                // Calculate data ptr to return
                uintptr_t data_ptr = uint_chk + align16(sizeof(hdr));

                // Align size of data to be /16
                size_t aligned_size = align16(alloc_size);
                curr_chk->free = 1;
                split_chunk(curr_chk, aligned_size);
                return (void *) data_ptr;
            }
        }
        curr_chk = curr_chk->next_chk;
    }
    // Need more data
    increase_heap(); // will have loop call this
    return add_chunk(alloc_size);
}

void split_chunk(hdr *chk, size_t alloc_size){
    // Only make a new header if it can fit into remainder
    if ((chk->size - alloc_size) > sizeof(hdr)){
        // Can add a header into remainder
        uintptr_t uint_chk = (uintptr_t) chk;
        uintptr_t uint_new_hdr = uint_chk + align16(sizeof(hdr)) + alloc_size;
        hdr *ptr_new_hdr = (hdr *) uint_new_hdr;    // Calculate new hdr ptr
        // set the new header info
        size_t rem_data = chk->size - alloc_size - align16(sizeof(hdr));
        if (chk->next_chk == NULL){
            // If adding to last node
            create_hdr(ptr_new_hdr, NULL, chk, rem_data);
            tail = uint_new_hdr;    // Update tail
        } else {
            // Not last node
            create_hdr(ptr_new_hdr, chk->next_chk, chk, rem_data);
        }
        chk->size = alloc_size;
        chk->next_chk = ptr_new_hdr;
    }
    return;
}

void increase_heap(){
    // Add 64k to the heap
    hdr *last_node = (hdr *) tail;
    void *old_brk;
    if ((old_brk = sbrk(HEAP_CHUNKS)) == (void *) -1){
        perror("Failed SBRK");
        exit(1);
    }

    if (last_node->free == 0){
        // Last node is free
        last_node->size += HEAP_CHUNKS;
    } else {
        // Last node not free
        uintptr_t new_hdr = align16(tail + sizeof(hdr) + last_node->size);
        create_hdr((hdr *) new_hdr, NULL, last_node, HEAP_CHUNKS);
        last_node->next_chk = (hdr *) new_hdr;
    }
}

void free(void * ptr){
    if (ptr == NULL){
        return;
    }
}

void *realloc(void *ptr, size_t size){
    return;
}

void *calloc(size_t nmemb, size_t size){
    return;
}






uintptr_t align16(uintptr_t addr){
    // Align address to be divisible by 16 and return value
    return (addr + 15) & ~15;   // Add by 15 to always round to up
}

void create_hdr(hdr *chk_hdr, hdr *next, hdr *prev, size_t alloc_size){
    // Create a new chunk header node for linked list
    chk_hdr->free = 0;
    chk_hdr->next_chk = next;
    chk_hdr->prev_chk = prev;
    chk_hdr->size = alloc_size;
    return;
}

uintptr_t init_heap(void){
    void *old_brk;
    if ((old_brk = sbrk(HEAP_CHUNKS)) == (void *) -1){
        char *s = "Failed SBRK";
        snprintf(buffer, BUFFER_SIZE, "%s\n", s);
        // write(2, buffer, BUFFER_SIZE);
        exit(1);
    }

    // Make floor divisible by 16
    uintptr_t uint_flr = align16((uintptr_t) old_brk);
    size_t flr_diff = uint_flr - (uintptr_t) old_brk;

    // char *s = "old_brk:";
    // snprintf(buffer, BUFFER_SIZE, "%s %d\n", s, uint_flr);
    // write(2, buffer, BUFFER_SIZE);

    hdr *ptr_flr = (hdr *) uint_flr;
    create_hdr(ptr_flr, NULL, NULL, (HEAP_CHUNKS) - flr_diff - sizeof(hdr));
    return (uintptr_t) ptr_flr;
}