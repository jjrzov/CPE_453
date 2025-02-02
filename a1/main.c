#include "malloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void){

    void *ptr = malloc(200);
    print_heap();
    pp(stdout, "Pointer: %p\n", ptr);
    realloc(ptr, 100);
    print_heap();
    return 0;
}