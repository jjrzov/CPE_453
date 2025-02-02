#include "dine.h"

#define P_SHARED    0
#define INIT_SEMA_VAL   1
#define HUNGRY  0
#define EATING  1
#define CHANGING 2

sem_t print_sema;
sem_t forks[NUM_PHILOSOPHERS];
int cycles = 1;
pthread_t philos[NUM_PHILOSOPHERS];
int ids[NUM_PHILOSOPHERS];
int philos_status[NUM_PHILOSOPHERS];

int main(int argc, char *argv[]){
    if (argc > 2){
        perror("ERROR: Invalid Input");
        exit(EXIT_FAILURE);
    }

    // Parse Number of Cycles from command line
    if (argc == 2){
        if (sscanf(argv[1], "%d", &cycles) == EOF){
            perror("ERROR: Invalid Input, not an int");
            exit(EXIT_FAILURE);
        }
    }

    // Check if there are enough philosophers
    if (NUM_PHILOSOPHERS < 2){
        exit(EXIT_FAILURE);
    }

    print_header();
    initSemaphores();
    initPhilosophers();

    waitPhilosophers(); // Wait for all philosophers to finish

    return 0; // Success

}

void initSemaphores(void){
    // Initialize Semaphores
    int retVal, i;

    // Printing Sema
    retVal = sem_init(&print_sema, P_SHARED, INIT_SEMA_VAL);
    if (retVal == -1){
        perror("ERROR: print Semaphore sem_init() Failed");
        exit(EXIT_FAILURE);
    }

    // Fork Semas
    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = sem_init(&forks[i], P_SHARED, INIT_SEMA_VAL);
        if (retVal == -1){
            perror("ERROR: fork Semaphores sem_init() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void initPhilosophers(void){
    // Initialize Philosophers
    int retVal, i;

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        ids[i] = 'A' + i; // Set id numbers
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        philos_status[i] = CHANGING;    // Philosophers start in changing state
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = pthread_create(&philos[i], NULL, dine, (void *) (&ids[i]));
        if (retVal == -1){
            perror("ERROR: pthread_create() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void waitPhilosophers(void){
    // Wait for all philosophers to end
    int retVal, i;

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = pthread_join(ids[i], NULL);
        if (retVal == -1){
            perror("ERROR: pthread_join() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void dine(void *id){
    int myID = *(int *) id;


    
    pthread_exit(NULL);
    return NULL;    // Exit thread
}

void pickUpForks(int philoID){
    // Have philopoher pick up a fork
    // Even ID philosophers pick up their right hand fork
    // Odd ID philosophers pick up their left hand fork
    int direction = philoID % 2;
    
}

void dawdle(void){
    /*
    * sleep for a random amount of time between 0 and 999
    * milliseconds. This routine is somewhat unreliable, since it
    * doesn’t take into account the possiblity that the nanosleep
    * could be interrupted for some legitimate reason.
    *
    * nanosleep() is part of the realtime library and must be linked
    * with –lrt
    */
    struct timespec tv;
    int msec = (int)(((double)random() / RAND_MAX) * 1000);

    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec;
    if (-1 == nanosleep(&tv, NULL)){
        perror("ERROR: nanosleep() Failed");
    }
}



