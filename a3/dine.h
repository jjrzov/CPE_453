#ifndef DINE_H
#define DINE_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS    5
#endif

void initSemaphores(void);
void initPhilosophers(void);
void dawdle(void);

#endif