#include "malloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void){
    // pp(stdout, "No Align: %d\n", sizeof(header));
    // pp(stdout, "Aligned: %d\n", align16(sizeof(header)));

    int i = 0;
    for(i = 0; i < 8192; i++){
        size_t size = i;
        pp(stdout, "%d\n", i);
        malloc(size);
        // print_heap();
    }
    return 0;
}