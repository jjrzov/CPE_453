#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <pp.h>

char buffer[50];

#define BUFFER_SIZE 50

int main(void){
    pp(stdout, "HELLO\n");
    pp(stdout, "Not Aligned: %d\n", sizeof(hdr));
    pp(stdout, "Aligned: %d\n", align16(sizeof(hdr)));
    malloc(64 * 1024);
    pp(stdout, "FINISHED\n");
    return 0;
}