#include "print.h"

#define CHAR_OFFSET 48
#define PADDING 8

void print_header(){
    // Print the header of Dine Table
    int i, j;
    int left_spacing, right_spacing;

    print_line();

    // Need dynamic value for spacing because amount of forks to print changes
    left_spacing = (NUM_PHILOSOPHERS + PADDING) / 2;
    if ((NUM_PHILOSOPHERS % 2) == 0){
        right_spacing = left_spacing - 1;
    } else {
        right_spacing = left_spacing;
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        printf("|");
        for (j = 0; j < left_spacing; j++){
            printf(" ");    // Printing left padding
        }

        printf("%c", 'A' + i);  // Print philosopher letter

        for (j = 0; j < right_spacing; j++){
            printf(" ");    // Printing right padding
        }
    }
    printf("|\n");

    print_line();
    return;
}

void print_status(){
    // Print main info line of fork status and state of each philosopher
    int i, j;
    int left_fork, right_fork;

    // Wait for print sema
    sem_wait(&printSema);

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        printf("| ");

        // Printing fork status
        for (j = 0; j < NUM_PHILOSOPHERS; j++){
            left_fork = i;
            right_fork = (i + RIGHT) % NUM_PHILOSOPHERS;

            if (left_fork == j && philos[i].left_fork == HAVE){
                printf("%c", (j + CHAR_OFFSET));
            } else if (right_fork == j && philos[i].right_fork == HAVE){
                printf("%c", (j + CHAR_OFFSET));
            } else {
                printf("-");
            }
        }

        // Printing state status
        if (philos[i].state == EATING){
            printf(" EAT   ");
        } else if (philos[i].state == THINKING){
            printf(" Think ");
        } else {
            printf("       ");
        }
    }

    printf("|\n");

    sem_post(&printSema);   // Give back printing sema

    return;
}

void print_footer(void){
    print_line();
}

void print_line(void){
    int i, j;

    printf("|");
    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        for (j = 0; j < (NUM_PHILOSOPHERS + PADDING); j++){
            printf("=");
        }

        printf("|");
    }

    printf("\n");
}