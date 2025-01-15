#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <pp.h>

#define BUFFER_SIZE 1500
#define STDERR  2

uintptr_t flr = NULL;
uintptr_t tail = NULL;

void *realloc(void *ptr, size_t size);
void free(void * ptr);
void *calloc(size_t nmemb, size_t size);
int find_scale(int num);

char buffer[BUFFER_SIZE];

void *malloc(size_t size){
    if (size == 0){
        return NULL;
    }
    if ((hdr *) flr == NULL){
        flr = init_heap();  // Need to initialize heap
        tail = flr; // Update tail
    }
    return add_chunk(size);

}

void *add_chunk(size_t req_size){
    hdr *curr_chk = (hdr *) flr;
    size_t aligned_size = align16(req_size);  // Align size of data
    
    while (curr_chk != NULL){
        if (curr_chk->free == 0){   // Current chunk is free
            if (curr_chk->size >= aligned_size){
                // Able to add to chunk
                uintptr_t uint_curr_chk = (uintptr_t) curr_chk;
                split_chunk(curr_chk, aligned_size);

                // Calculate data address within chunk
                uintptr_t data_ptr = uint_curr_chk + align16(sizeof(hdr));
                return (void *) data_ptr;
            }
        }
        curr_chk = curr_chk->next_chk;
    }
    // Need more data
    increase_heap(aligned_size); // will have loop call this
    return add_chunk(req_size);
}

void split_chunk(hdr *chk, size_t des_size){
    // Splits the given chunk into two if possible of desired size and remainder

    // Only make a new header if it can fit into remainder
    if ((chk->size - des_size) > align16(sizeof(hdr))){

        // Can add a header into remainder
        uintptr_t uint_chk = (uintptr_t) chk;
        uintptr_t new_hdr = uint_chk + align16(sizeof(hdr)) + des_size;

        // Set the new header info
        size_t rem_data = chk->size - des_size - align16(sizeof(hdr));
        if (chk->next_chk == NULL){
            // If adding to last node
            create_hdr((hdr *) new_hdr, NULL, chk, rem_data);
            tail = new_hdr;    // Update tail
        } else {
            // Not last node
            create_hdr((hdr *) new_hdr, chk->next_chk, chk, rem_data);
        }
        chk->size = des_size;
        chk->next_chk = (hdr *) new_hdr;
    }

    // If no space for header keep chunk size the same
    // Only thing that changes is chunk no longer free
    chk->free = 1; 
    return;
}

void increase_heap(size_t size){
    // Add 64k * n to the heap
    int n = find_scale(size);   // Find how many multiples of 64k is needed

    void *old_brk;
    if ((old_brk = sbrk(n * HEAP_INCR)) == (void *) -1){
        pp(stderr, "Failed SBRK\n");
        exit(1);
    }

    hdr *last_node = (hdr *) tail;
    if (last_node->free == 0){
        // Last node is free
        last_node->size += (n * HEAP_INCR);
    } else {
        // Last node not free
        uintptr_t new_hdr = tail + align16(sizeof(hdr)) + last_node->size;
        create_hdr((hdr *) new_hdr, NULL, last_node, n * HEAP_INCR);
        last_node->next_chk = (hdr *) new_hdr;
        tail = new_hdr;
    }
}

int find_scale(int num){
    // Compute value of scalar for increasing heap
    int n = ((num + HEAP_INCR - 1)/ (HEAP_INCR));
    if (n < 1){
        return 1;
    }
    return n;
}

void free(void *ptr){
    if (ptr == NULL){
        return;
    }

}

void *realloc(void *ptr, size_t size){
    return NULL;
}

void *calloc(size_t nmemb, size_t size){
    return NULL;
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
    if ((old_brk = sbrk(HEAP_INCR)) == (void *) -1){
        pp(stderr, "Failed SBRK\n");
        exit(1);
    }

    // Make floor divisible by 16
    uintptr_t uint_flr = align16((uintptr_t) old_brk);
    size_t flr_diff = uint_flr - (uintptr_t) old_brk;
    hdr *ptr_flr = (hdr *) uint_flr;
    size_t size = (HEAP_INCR) - flr_diff - align16(sizeof(hdr));
    create_hdr(ptr_flr, NULL, NULL, size);
    return (uintptr_t) ptr_flr;
}