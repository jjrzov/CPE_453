#include <stdio.h>
#include "header.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

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
    int *old_brk;
    if ((old_brk = sbrk(HEAP_CHUNKS)) == (void *) -1){
        perror("Failed SBRK");
        exit(1);
    }

    // Make floor divisible by 16
    uintptr_t uint_flr = align16((uintptr_t) old_brk);
    size_t flr_diff = uint_flr - (uintptr_t) old_brk;
    hdr *ptr_flr = (hdr *) uint_flr;
    create_hdr(ptr_flr, NULL, NULL, (HEAP_CHUNKS) - flr_diff - sizeof(hdr));
    return ptr_flr;
}

int main(int argc, char *argv[]){
    init_heap();
}