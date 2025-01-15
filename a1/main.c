#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    my_malloc(1024);
    print_heap();
    return 0;
}





















// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>

// void fill(unsigned char *s, size_t size, int start) {
//   int i;
//   for(i=0; i< size; i++)
//     s[i]=start++;
// }

// int check(unsigned char *s, size_t size, int start){
//   int i,err=0;
//   for(i=0; i < size; i++,start++)
//     err += (s[i] != (start&0xff));

//   return err;
// }

// void *getbuff(size_t size){
//   unsigned char *p;
//   p = malloc(size);
//   if ( p )
//     printf("Calling malloc succeeded.\n");
//   else {
//     printf("malloc() returned NULL.\n");
//     exit(1);
//   }

//   fill(p,size,0);
//   printf("Successfully used the space.\n");

//   if ( check(p,size,0) )
//     printf("Some values didn't match in region %p.\n",p);

//   return p;
// }


// int main(int argc, char *argv[]){
//   unsigned char *val;
//   int i,err;

//   printf("Allocating 8192 regions, size 0..8191...");
//   err=0;
//   for(i=0;i < 8192 && !err ;i++ ) {
//     size_t size = i;

//     val = malloc (size);
//     if ( !val && size ) {
//       fprintf(stderr,"malloc(%d) failed: 0x%p\n",
//               i,val);
//       err++;
//     } else {
//       /* check to see if the returned address lines up mod 16 */
//       if ( (long)val & (long)0xf ) {
//         fprintf(stderr,"malloc(%d) returned unaligned pointer: 0x%p\n",
//                 i,val);
//         err++;
//         break;
//       }
//     }
//   }
//   if ( !err )
//     printf("ok.\n");
//   else
//     printf("FAILED.\n");
//   exit(0);
// }
