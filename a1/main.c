#include "malloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void){

    void *ptr1 = malloc(500);
    void *ptr2 = malloc(1000);
    void *ptr3 = malloc(1500);
    void *ptr4 = malloc(2000);
    void *ptr5 = malloc(2500);
    print_heap();

    free(ptr1);
    print_heap();
    free(ptr2);
    print_heap();
    free(ptr3);
    print_heap();
    free(ptr4);
    print_heap();
    free(ptr5);
    print_heap();
    return 0;
}