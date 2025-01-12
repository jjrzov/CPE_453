#include <stdio.h>
#include "header.h"

uintptr_t flr = NULL;
uintptr_t tail = NULL;

uintptr_t my_malloc(size_t size);

uintptr_t my_malloc(size_t size){
    if (flr == NULL){
        flr = init_heap();  // Need to initialize heap
        tail = flr; // Update tail
    }

    add_chunk(size);
}

void *add_chunk(size_t alloc_size){
    hdr *curr_chk = (hdr *) flr;
    while (curr_chk != NULL){
        if (curr_chk->free == 0){   // Current chunk is free
            if (curr_chk->size > alloc_size){
                // Able to add to chunk
                uintptr_t uint_chk = (uintptr_t) curr_chk;
                // Calculate data ptr to return
                uintptr_t data_ptr = align16(uint_chk + sizeof(hdr));

                // Align size of data to be /16
                size_t aligned_size = align16(alloc_size);
                curr_chk->free = 1;
                split_chunk(curr_chk, aligned_size);
                return (void *) data_ptr;
            }
        }
    }
    // Need more data
    increase_heap();
    return add_chunk(alloc_size);
}

void split_chunk(hdr *chk, size_t alloc_size){
    // Only make a new header if it can fit into remainder
    if ((chk->size - alloc_size) > sizeof(hdr)){
        // Can add a header into remainder
        uintptr_t uint_chk = (uintptr_t) chk;
        uintptr_t uint_new_hdr = align16(uint_chk + sizeof(hdr) + alloc_size);
        hdr *ptr_new_hdr = (hdr *) uint_new_hdr;    // Calculate new hdr ptr
        size_t rem_data = chk->size - alloc_size - sizeof(hdr);
        if (chk->next_chk == NULL){
            // If adding to last node
            create_hdr(ptr_new_hdr, NULL, chk, rem_data);
            chk->size = alloc_size;
            chk->next_chk = ptr_new_hdr;
            tail = uint_new_hdr;    // Update tail
        } else {
            // Not last node
            create_hdr(ptr_new_hdr, chk->next_chk, chk, rem_data);
            chk->size = alloc_size;
            chk->next_chk = ptr_new_hdr;
        }
    }
    return;
}

void increase_heap(){
    // Add 64k to the heap
    hdr *last_node = (hdr *) tail;
    int *old_brk;
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