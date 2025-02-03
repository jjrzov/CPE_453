#include "dine.h"

#define P_SHARED    0
#define INIT_SEMA_VAL   1

sem_t printSema;    // Mutex for printing
sem_t forks[NUM_PHILOSOPHERS];
int cycles = 1; // Default values for cycles
philo philos[NUM_PHILOSOPHERS];


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

    print_footer();

    tearDownSemaphores();   // Clean up program
    return 0; // Success

}

void initSemaphores(void){
    // Initialize Semaphores
    int retVal, i;

    // Printing Sema
    retVal = sem_init(&printSema, P_SHARED, INIT_SEMA_VAL);
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
        philos[i].id = 'A' + i; // Set id numbers
        philos[i].state = CHANGING;    // Philosophers start in changing state
        philos[i].left_fork = DONT_HAVE;
        philos[i].right_fork = DONT_HAVE;
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = pthread_create(&philos[i].thread, NULL, dine, 
                                    (void *) (&philos[i]));

        if (retVal == -1){
            perror("ERROR: pthread_create() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void *dine(void *arg){
    // Philosophers loop through eating and thinking for cycles amount of times
    philo *phil = (philo *) arg;
    int philo_index = phil->id - 'A';   // Get index in philosopher list from id
    print_status(); // Inital print
    
    int i;
    for (i = 0; i < cycles; i++){
        eat(philo_index);  // Try and eat      

        philos[philo_index].state = CHANGING;   // Changing from eat to think
        print_status();

        think(philo_index);    // Drop forks and start thinking
    }

    philos[philo_index].state = CHANGING;   // Philosopher ends in changing
    print_status();

    pthread_exit(NULL);
    return NULL;    // Exit thread
}

void eat(int p_index){
    // Philosopher needs both forks to eat
    
    // Not yet eating thus philosopher is currently changing states
    philos[p_index].state = CHANGING;
    print_status();

    // Even philosophers take right fork first
    // Odd philosophers take left fork first
    int dir = p_index % 2;
    int right_hand = (p_index + RIGHT) % NUM_PHILOSOPHERS;
    int left_hand = p_index;

    if (dir == 0){
        // Even philosopher
        takeFork(p_index, right_hand, RIGHT);
        takeFork(p_index, left_hand, LEFT);

    } else {
        // Odd philosopher
        takeFork(p_index, left_hand, LEFT);
        takeFork(p_index, right_hand, RIGHT);
    }
        
    philos[p_index].state = EATING; // Now has both forks thus eating
    print_status();

    dawdle();   // Eat for random amount of time
    print_status();

    return;
}

void takeFork(int p_index, int f_index, int side){
    // Take given fork and update the status
    sem_wait(&forks[f_index]);  // Decrement the specfic fork semaphore

    // Update which fork was taken
    if (side == RIGHT){
        philos[p_index].right_fork = HAVE;
    } else if (side == LEFT){
        philos[p_index].left_fork = HAVE;
    }

    print_status();
    return;
}

void think(int p_index){
    // Place down both forks one at a time before thinking
    int right_hand = (p_index + RIGHT) % NUM_PHILOSOPHERS;
    int left_hand = (p_index);

    // Arbitrarily give right fork first
    giveFork(p_index, right_hand, RIGHT);
    giveFork(p_index, left_hand, LEFT);

    philos[p_index].state = THINKING;   // Dropped both forks thus can think
    print_status();

    dawdle();   // Think for random amount of time
    print_status();

    return;
}

void giveFork(int p_index, int f_index, int side){
    // Give back fork philosopher has and update status
    
    // Update status then return semaphore
    if (side == RIGHT){
        philos[p_index].right_fork = DONT_HAVE;
    } else {
        philos[p_index].left_fork = DONT_HAVE;
    }

    sem_post(&forks[f_index]);  // Give sema back

    print_status();
    return;
}

void waitPhilosophers(void){
    // Wait for all philosophers to end
    int retVal, i;

    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = pthread_join(philos[i].thread, NULL);
        if (retVal == -1){
            perror("ERROR: pthread_join() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void tearDownSemaphores(void){
    // Destroy Semaphores
    int retVal, i;

    // Printing Sema
    retVal = sem_destroy(&printSema);
    if (retVal == -1){
        perror("ERROR: print Semaphore sem_destroy() Failed");
        exit(EXIT_FAILURE);
    }

    // Fork Semas
    for (i = 0; i < NUM_PHILOSOPHERS; i++){
        retVal = sem_destroy(&forks[i]);
        if (retVal == -1){
            perror("ERROR: fork Semaphores sem_destroy() Failed");
            exit(EXIT_FAILURE);
        }
    }

    return;
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



