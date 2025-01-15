#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
// #include <pp.h>

#define BUFFER_SIZE 1500
#define STDERR  2

hdr *flr = NULL;
hdr *tail = NULL;

char buffer[BUFFER_SIZE];

void *malloc(size_t size){
    printf("Calling Malloc\n");
    if (size == 0){
        return NULL;
    }
    if (flr == NULL){
        if ((flr = init_heap()) == NULL){ // Need to initialize heap
            return NULL;
        }
        tail = flr; // Update tail
    }
    return add_chunk(size);

}

void *add_chunk(size_t req_size){
    printf("Add Chunk\n");
    hdr *curr_chk = flr;
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
    if (increase_heap(aligned_size) == -1){
        // NEED TO SET ERRNO TO ENOMEM
        return NULL;
    }
    return add_chunk(req_size);
}

void split_chunk(hdr *chk, size_t des_size){
    printf("Split Chunk\n");
    // Splits the given chunk into two if possible of desired size and remainder

    // Only make a new header if it can fit into remainder
    if ((chk->size - des_size) > align16(sizeof(hdr))){

        // Can add a header into remainder
        uintptr_t uint_chk = (uintptr_t) chk;
        hdr *new_hdr = (hdr *) (uint_chk + align16(sizeof(hdr)) + des_size);
        // Set the new header info
        size_t rem_data = chk->size - des_size - align16(sizeof(hdr));

        new_hdr->free = 0;  // Create new header
        new_hdr->next_chk = chk->next_chk;
        new_hdr->prev_chk = chk;
        new_hdr->size = rem_data;

        if (chk->next_chk == NULL){
            printf("Last Node\n");
            tail = new_hdr;
        } else {
            chk->next_chk->prev_chk = new_hdr;
        }

        chk->size = des_size;
        chk->next_chk = new_hdr;
    }

    // If no space for header keep chunk size the same
    // Only thing that changes is chunk no longer free
    chk->free = 1; 
    return;
}

int increase_heap(size_t size){
    printf("Increasing Heap\n");
    // Add 64k * n to the heap
    int n = find_scale(size);   // Find how many multiples of 64k is needed

    void *old_brk;
    if ((old_brk = sbrk(n * HEAP_INCR)) == (void *) -1){
        return -1;
    }

    if (tail->free == 0){
        // Last node is free
        tail->size += (n * HEAP_INCR);
    } else {
        // Last node not free
        uintptr_t new_hdr = (uintptr_t)tail + align16(sizeof(hdr)) + tail->size;

        // create_hdr((hdr *) new_hdr, NULL, tail, n * HEAP_INCR);
        hdr *hdr_ptr = (hdr *) new_hdr;
        hdr_ptr->free = 0;
        hdr_ptr->next_chk = NULL;
        hdr_ptr->prev_chk = tail;
        hdr_ptr->size = (n * HEAP_INCR);

        tail->next_chk = (hdr *) new_hdr;
        tail = (hdr *) new_hdr; // Update tail
    }
    return 1;
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
    printf("Creating new header\n");
    // Create a new chunk header node for linked list
    chk_hdr->free = 0;
    chk_hdr->next_chk = next;
    chk_hdr->prev_chk = prev;
    chk_hdr->size = alloc_size;
    return;
}

hdr *init_heap(void){
    void *old_brk;
    if ((old_brk = sbrk(HEAP_INCR)) == (void *) -1){
        // NEED TO SET ERRNO TO ENOMEM
        return NULL;
    }

    // Make floor divisible by 16
    uintptr_t uint_flr = align16((uintptr_t) old_brk);
    size_t flr_diff = uint_flr - (uintptr_t) old_brk;
    hdr *ptr_flr = (hdr *) uint_flr;
    size_t size = (HEAP_INCR) - flr_diff - align16(sizeof(hdr));
    
    // create_hdr(ptr_flr, NULL, NULL, size);
    ptr_flr->free = 0;
    ptr_flr->next_chk = NULL;
    ptr_flr->prev_chk = NULL;
    ptr_flr->size = size;
    
    return ptr_flr;
}

void print_heap(void){
    hdr *curr_chk = flr;
    printf("############################\n");
    while (curr_chk != NULL){
        printf("-------------------\n");
        printf("Chunk Address: %p\n", curr_chk);
        printf("Prev Address: %p\n", curr_chk->prev_chk);
        printf("Next Address: %p\n", curr_chk->next_chk);
        printf("Size: %ld\n", curr_chk->size);
        printf("Free Status: %d\n", curr_chk->free);
        printf("-------------------\n\n");

        curr_chk = curr_chk->next_chk;
    }
}

int main(void){
    for(int i = 0; i < 8192; i++){
        if (i ==  5281){
            print_heap();
            printf("5281 Aligned: %ld\n", align16(5281));
        }
        size_t size = i;
        printf("%ld\n", size);
        my_malloc(size);
    }
    return 0;
}