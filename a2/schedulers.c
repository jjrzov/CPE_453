#include "schedulers.h"

/* Will be using sched_one and sched_two pointers to store next and prev
    for circular doubly linked list*/
#define sched_next  sched_one
#define sched_prev  sched_two

struct scheduler_info{
    thread head;
    thread tail;
    int count;
};

void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next(void);
int qlen(void);

struct scheduler_info schdlr_info = {NULL, NULL, 0};
struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next, qlen};
scheduler RoundRobin = &rr_publish;


void rr_admit(thread new){
    // Add the passed thread into scheduler pool
    if (schdlr_info.count == 0){
        // Adding new thread to an empty list
        new->sched_next = new;
        new->sched_prev = new;

        // Update scheduler info
        schdlr_info.head = new;
        schdlr_info.tail = new;
        schdlr_info.count = 1;
    } else {
        // Adding node to end of list
        new->sched_next = schdlr_info.head;
        new->sched_prev = schdlr_info.tail;

        schdlr_info.head->sched_prev = new;
        schdlr_info.tail->sched_next = new;
        schdlr_info.tail = new;
        schdlr_info.count++;
    }
    return;
}

void rr_remove(thread victim){
    // Remove the passed thread from scheduler pool
    if (schdlr_info.count == 0){
        return;    // No threads in list
    } else if (schdlr_info.count == 1){
        schdlr_info.head = NULL;    // Only one thing in list
        schdlr_info.tail = NULL;
        schdlr_info.count--;
        return;
    }

    if (victim == schdlr_info.head){
        // Removing the head
        schdlr_info.head->sched_next->sched_prev = schdlr_info.tail;
        schdlr_info.tail->sched_next = schdlr_info.head->sched_next;

        schdlr_info.head = victim->sched_next;  // New head

    } else if (victim == schdlr_info.tail){
        // Removing the tail
        schdlr_info.head->sched_prev = schdlr_info.tail->sched_prev;
        schdlr_info.tail->sched_prev->sched_next = schdlr_info.head;
        
        schdlr_info.tail = victim->sched_prev;  // New tail
    
    } else {
        // Adding somewhere inbetween
        victim->sched_next->sched_prev = victim->sched_prev;
        victim->sched_prev->sched_next = victim->sched_next;
    
    }
    
    schdlr_info.count--;    // Removed a thread
    return;
}

thread rr_next(void){
    // Return the next thread to run
    if (schdlr_info.count == 0){
        return NULL;    // No threads in list
    }

    // Move thread to be run to the end of the line and shift to new head
    schdlr_info.head = schdlr_info.head->sched_next;
    schdlr_info.tail = schdlr_info.head->sched_prev;

    return schdlr_info.tail;
}

int qlen(void){
    // Return number of active threads in scheduler
    return schdlr_info.count;
}
