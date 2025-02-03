#ifndef DINE_H
#define DINE_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "print.h"

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS    5
#endif

#define DONT_HAVE   0
#define HAVE   1

#define LEFT    0
#define RIGHT   1

#define THINKING  0
#define EATING  1
#define CHANGING 2

typedef struct philosopher{
    int id;
    int right_fork;
    int left_fork;
    int state;
    pthread_t thread;
} philo;

extern sem_t printSema;
extern sem_t forks[NUM_PHILOSOPHERS];
extern philo philos[NUM_PHILOSOPHERS];

void initSemaphores(void);
void initPhilosophers(void);
void *dine(void *id);
void eat(int id);
void takeFork(int philo_id, int fork_id, int side);
void think(int id);
void giveFork(int philo_id, int fork_id, int side);
void waitPhilosophers(void);
void tearDownSemaphores(void);
void dawdle(void);

#endif