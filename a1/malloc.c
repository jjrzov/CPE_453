#include "malloc.h"

int init_heap();
int align16(int val);
header *find_free_chunk(size_t req_size);
int increase_heap(size_t req_size);
void split_chunk(header *chk, size_t req_size);
int find_scalar(int val);
header *ptr_to_chunk(header *ptr);
void merge_neighbors(header *chk);
uintptr_t get_chk_data(header *chk);
uintptr_t get_chk_end(header *chk);

header *head = NULL;
header *tail = NULL;

void *malloc(size_t size){
    if (head == NULL){
        // Need to initalize heap
        if (init_heap() != HEAP_CREATED){
            pp(stderr, "MALLOC: No Memory Available\n");
            return NULL;
        }
    }
    
    if (size <= 0){
        if (getenv("DEBUG_MALLOC")){
            pp(stderr, "MALLOC: malloc(%d)\t=>\t(ptr=%p, size=%d)\n", size, 
                        NULL, size);
        }
        return NULL;
    }

    size_t aligned_size = align16(size);

    header *free_chk = find_free_chunk(aligned_size);
    if (free_chk == NULL){
        // Need to increase heap
        int retval = increase_heap(aligned_size);
        if (retval != HEAP_INCREASED){
            pp(stderr, "MALLOC: No Memory Available\n");
            return NULL;
        }
        free_chk = tail;    // Tail was updated to fit requested size
    }

    split_chunk(free_chk, aligned_size);
    if (getenv("DEBUG_MALLOC")){
        pp(stderr, "MALLOC: malloc(%d)\t=>\t(ptr=%p, size=%d)\n", size, 
                    free_chk, aligned_size);
    }

    return (void *)get_chk_data(free_chk);
}

void free(void *ptr){
    if (ptr == NULL){
        return;
    }

    // Find which chunk pointer is referencing 
    header *chk = ptr_to_chunk((header *)ptr);
    if (chk == NULL){
        pp(stderr, "FREE: Error Invalid Pointer\n");
        return;
    }

    if (chk->is_free == 1){
        // Chunk already free
        pp(stderr, "FREE: Error Pointer Previously Called\n");
        return;
    }

    // Merge neighboring chunks if free
    merge_neighbors(chk);
    if (getenv("DEBUG_MALLOC")){
        pp(stderr, "MALLOC: free(%p)\n", ptr);
    }
}

void *realloc(void *ptr, size_t size){
    if (head == NULL){
        // Need to initalize heap
        if (init_heap() != HEAP_CREATED){
            pp(stderr, "REALLOC: No Memory Available\n");
            return NULL;
        }
    }

    if (ptr == NULL){
        void *data_ptr = malloc(size);
        if (data_ptr == NULL){
            pp(stderr, "REALLOC: Failed to Allocate Data\n");
            return NULL;
        }
        
        if (getenv("DEBUG_MALLOC")){
            pp(stderr, "MALLOC: realloc(%p,%d)\t=>\t(ptr=%p, size=%d)\n", ptr, 
                        size, ptr, size);
        }
        return data_ptr;
    }

    if (size == 0){
        free(ptr);
        if (getenv("DEBUG_MALLOC")){
            pp(stderr, "MALLOC: realloc(%p,%d)\t=>\t(ptr=%p, size=%d)\n", ptr, 
                        size, ptr, size);
        }
        return NULL;
    }

    // Find which chunk pointer is referencing 
    header *chk = ptr_to_chunk((header *)ptr);
    if (chk == NULL){
        pp(stderr, "REALLOC: Error Invalid Pointer\n");
        return NULL;
    }

    size_t aligned_size = align16(size);

    if (chk->size > aligned_size){
        // Shrinking
        split_chunk(chk, aligned_size);
    } 
    
    else if (chk->size == aligned_size){
        // Requesting same size so don't need to change anything
        if (getenv("DEBUG_MALLOC")){
            pp(stderr, "MALLOC: realloc(%p,%d)\t=>\t(ptr=%p, size=%d)\n", ptr, 
                        size, chk, chk->size);
        }
        return ptr;
    } 
    
    else if (chk->next == NULL){
        // Expanding in last node
        int n = find_scalar(aligned_size);

        void *old_brk;  // Need to increase heap size
        if ((old_brk = sbrk(n * HEAP_INCR)) == (void *)-1){
            errno = ENOMEM;
            pp(stderr, "REALLOC: No Memory Available\n");
            return NULL;
        }

        tail->size += (n * HEAP_INCR);  // Add new space to tail
        split_chunk(tail, aligned_size);
    }

    else if (chk->next->is_free == 1 && aligned_size < chk->size + 
                                align16(HEADER_SIZE) + chk->next->size){
        // Expanding in a sandwiched chunk with space in next
        chk->size += align16(HEADER_SIZE) + chk->next->size;
        if (chk->next->next != NULL){
            chk->next->next->prev = chk;
        }
        chk->next = chk->next->next;
        split_chunk(chk, aligned_size);
    }

    else {
        // Expanding in a sandwiched chunk with no space
        void *data_ptr = malloc(aligned_size);
        if (data_ptr == NULL){
            pp(stderr, "REALLOC: Failed to Allocate Data\n");
            return NULL;
        }

        // Move data to new chunk
        memmove(data_ptr, (void *) get_chk_data(chk), chk->size);
        free(chk);  // Free old chunk where data used to be

        if (getenv("DEBUG_MALLOC")){
            header *new_chk = (header *) (
                          (uintptr_t) data_ptr - align16(HEADER_SIZE));
            pp(stderr, "MALLOC: realloc(%p,%d)\t=>\t(ptr=%p, size=%d)\n", ptr, 
                        size, new_chk, new_chk->size);
        }

        return data_ptr;
    }
    
    if (getenv("DEBUG_MALLOC")){
        pp(stderr, "MALLOC: realloc(%p,%d)\t=>\t(ptr=%p, size=%d)\n", ptr, 
                   size, chk, chk->size);
    }

    return ptr;

}

void *calloc(size_t nmemb, size_t size){
    if (head == NULL){
        // Need to initalize heap
        if (init_heap() != HEAP_CREATED){
            pp(stderr, "CALLOC: No Memory Available\n");
            return NULL;
        }
    }

    void *data_ptr = malloc(nmemb * size);
    if (data_ptr != NULL){

        if (getenv("DEBUG_MALLOC")){
            header *new_chk = (header *) 
                            ((uintptr_t) data_ptr - align16(HEADER_SIZE));
            pp(stderr, "MALLOC: calloc(%d,%d)\t=>\t(ptr=%p, size=%d)\n", nmemb, 
                        size, new_chk, new_chk->size);
        }

        return memset(data_ptr, 0, size);   // Clear allocated data
    }
    pp(stderr, "CALLOC: Failed to Allocate Data\n");
    return NULL;
}

int init_heap(){
    // Initialize the heap to 64k and initialize linked list
    void *old_brk;
    if ((old_brk = sbrk(HEAP_INCR)) == (void *) -1){
        errno = ENOMEM;
        return NO_MEMORY;
    }

    // Offset the floor to be divisible by 16
    uintptr_t flr = align16((uintptr_t)old_brk);
    uintptr_t flr_offset = flr - (uintptr_t)old_brk;

    head = (header *) flr;  // Initialize head and tail globals
    tail = (header *) flr;

    head->next = NULL;  // Create first header
    head->prev = NULL;
    head->is_free = 1;
    head->size = HEAP_INCR - flr_offset - align16(HEADER_SIZE);

    return HEAP_CREATED;
}

int align16(int val){
    // Align address to be divisible by 16 and return value
    return (val + 15) & ~15;   // Add by 15 to always round to up
}

header *find_free_chunk(size_t req_size){
    // Traverse through linkedlist for free chunk of requested size
    header *chk = head;

    while (chk != NULL){
        if (chk->is_free == 1 && chk->size >= req_size){
            return chk;
        }
        chk = chk->next;
    }
    return NULL;
}

int increase_heap(size_t req_size){
    // Increase heap size by n * 64k
    int n = find_scalar(req_size);

    void *old_brk;
    if ((old_brk = sbrk(n * HEAP_INCR)) == (void *)-1){
        errno = ENOMEM;
        return NO_MEMORY;
    }

    if (tail->is_free == 1){
        // If tail is free then can just add space to tail
        tail->size += (n * HEAP_INCR);
    } else {
        // Need to make new header for new space
        header * new_hdr = (header *)get_chk_end(tail);
        new_hdr->next = NULL;
        new_hdr->prev = tail;
        new_hdr->is_free = 1;
        new_hdr->size = (n * HEAP_INCR) - align16(HEADER_SIZE);

        tail->next = new_hdr;   // Update tail
        tail = new_hdr;
    }
    return HEAP_INCREASED;
}

void split_chunk(header *chk, size_t req_size){
    /* 
    Split Chunk into two with the first chunk being of size req_size. 
    The second chunk will be the size of the remainder, but if header cannot 
    fit into remainder size then chunk not split and space given to first chunk
    */ 

    uintptr_t chk_addr = (uintptr_t)chk;
    size_t tot_rem = chk->size - req_size;
    if (tot_rem > align16(HEADER_SIZE)){    // MAYBE >=???
        // Can fit a new header
        header *new_hdr = (header *)
                            (chk_addr + align16(HEADER_SIZE) + req_size);
        
        new_hdr->next = chk->next;  // Set new header info
        new_hdr->prev = chk;
        new_hdr->is_free = 1;
        new_hdr->size = tot_rem - align16(HEADER_SIZE);
        
        // Update neighbor chunks
        if (chk->next == NULL){
            tail = new_hdr;
        } else {
            chk->next->prev = new_hdr;
        }

        chk->size = req_size;
        chk->next = new_hdr;
    }
    // If remainder did not fit a header give space to first chunk
    chk->is_free = 0;

    return;
}

int find_scalar(int val){
    // Calculate how many increments of 64k need to be allocated
    int ret = (val + HEAP_INCR - 1) / HEAP_INCR;
    if (ret < 1){
        ret = 1;
    }
    return ret;
}

header *ptr_to_chunk(header *ptr){
    // Converts pointer to the chunk it resides in
    header *chk = head;

    while (chk != NULL){
        if (ptr >= chk && ptr < (header *)get_chk_end(chk)){
            return chk;
        }
        chk = chk->next;
    }
    return NULL;
}

void merge_neighbors(header *chk){
    // Merge neighboring chunks together if they are free
    chk->is_free = 1;

    // Merging Next
    if (chk->next != NULL){
        if (chk->next->is_free == 1){
            chk->size += align16(HEADER_SIZE) + chk->next->size;
            if (chk->next->next != NULL){
                chk->next->next->prev = chk;
            }

            chk->next = chk->next->next;
        }
    }

    // Merging Prev
    if (chk->prev != NULL){
        if (chk->prev->is_free == 1){
            chk->prev->size += align16(HEADER_SIZE) + chk->size;
            if (chk->next != NULL){
                chk->next->prev = chk->prev;
            }

            chk->prev->next = chk->next;
        }
    }
    return;
}

uintptr_t get_chk_data(header *chk){
    // Return address of the chunk's data
    return (uintptr_t)chk + align16(HEADER_SIZE);
}

uintptr_t get_chk_end(header *chk){
    // Return address of the end of the chunk
    return (uintptr_t)chk + align16(HEADER_SIZE) + chk->size;
}

void print_heap(void){
    // Prints out linked list of headers
    header *curr_chk = head;
    pp(stdout, "############################\n");
    while (curr_chk != NULL){
        pp(stdout, "-------------------\n");
        pp(stdout, "Chunk Address: %p\n", curr_chk);
        pp(stdout, "Prev Address: %p\n", curr_chk->prev);
        pp(stdout, "Next Address: %p\n", curr_chk->next);
        pp(stdout, "Size: %ld\n", curr_chk->size);
        pp(stdout, "Free Status: %d\n", curr_chk->is_free);
        pp(stdout, "-------------------\n\n");

        curr_chk = curr_chk->next;
    }
}
