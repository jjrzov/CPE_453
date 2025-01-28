#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include "util.h"
#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <stdint.h>


// Free lib pointer for pointing to waiting threads
#define lib_waiting_next    lib_one
// Free lib pointer for pointing to the head of the list of all threads
#define lib_thread_next    lib_two

#define DEFAULT_STACK_SIZE  (8 * 1024 * 1024)   // 8MB
#define WORD    sizeof(size_t)

// Global Variables
static scheduler sched = &rr_publish;
static unsigned long tid_counter = 0;
static thread total_threads = NULL;
static thread waiting_threads = NULL;
static thread exiting_threads = NULL;
static tid_t main_tid = 0;
static thread curr_thread = NULL;

// Helper Functions
static void lwp_wrap(lwpfun fun, void *arg);
static void add2TotalList(thread thr);
void add2WaitList(thread add_thr);
void add2ExitList(thread add_thr);
void removeFromWaitList(void);
void removeFromExitList(void);
void removeFromTotalList(thread oldest_thr);

static void lwp_wrap(lwpfun fun, void *arg){
    /* Call the given lwpfunction with the given argument.
    * Calls lwp exit() with its return value
    */
    int rval;
    rval = fun(arg);
    lwp_exit(rval);
}

tid_t lwp_create(lwpfun fun, void *arg){
    // Create a new thread and set up context for it
    thread new_thr = (thread) malloc(sizeof(context));  // Create new thread
    if (new_thr == NULL){
        perror("ERROR: malloc() Failed");
        return NO_THREAD;
    }

    new_thr->tid = ++tid_counter;   // Assign tid and increment counter
    new_thr->status = MKTERMSTAT(LWP_LIVE, 0);
    new_thr->exited = NULL;

    // Get the stack size of new thread
    size_t stack_size;
    long page_size;
    struct rlimit rlim;

    if (getrlimit(RLIMIT_STACK, &rlim) != 0){
        perror("ERROR: getrlimit() Failed");
        exit(EXIT_FAILURE);
    }

    if (rlim.rlim_cur != RLIM_INFINITY){
        stack_size = rlim.rlim_cur;
        
        // Round up to a multiple of page size
        if ((page_size = sysconf(_SC_PAGE_SIZE)) == -1){
            perror("ERROR: sysconf() Failed");
            exit(EXIT_FAILURE);
        }
        stack_size = ((stack_size + page_size - 1) / page_size) * page_size; 

    } else {
        stack_size = DEFAULT_STACK_SIZE;
    }

    // Add the stack to new thread
    void *init_stk_ptr = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, 
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
                            -1, 0);
    
    if (init_stk_ptr == MAP_FAILED){
        perror("ERROR: mmap() Failed");
        free(new_thr);
        return NO_THREAD;
    }

    // Set up stack
    new_thr->stack = (unsigned long *) init_stk_ptr;    
    // TODO: COMPARE DIFFERENCES WITH ADDING STACK & ALIGN16
    new_thr->stacksize = stack_size;

    // Compute new top of stack pointer
    uintptr_t saved_bp = (uintptr_t) init_stk_ptr;
    uintptr_t base_ptr = ((uintptr_t) init_stk_ptr) + stack_size - WORD;
    unsigned long *stack = (unsigned long *) base_ptr;

    // stack[0] = (unsigned long) lwp_exit;   //                         ???
    stack[-1] = (unsigned long) lwp_wrap;   // Return Address
    stack[-2] = (unsigned long) saved_bp;    // "Saved" Base Pointer

    // Initialize register file
    rfile *regs = &new_thr->state;
    uintptr_t sp = base_ptr - WORD - WORD;   // Stack pointer
    regs->rdi = (unsigned long) fun;
    regs->rsi = (unsigned long) arg;
    regs->rbp = (unsigned long) sp;
    regs->rsp = (unsigned long) sp;
    regs->fxsave = FPU_INIT;

    sched->admit(new_thr); // Admit to scheduler
    add2TotalList(new_thr); // Keep track of all threads for tid2thread
    return new_thr->tid;
}

static void add2TotalList(thread add_thread){
    // Add a thread into the total list of all threads created
    if (total_threads == NULL){
        // Initializing total thread list
        total_threads = add_thread;
        add_thread->lib_thread_next = NULL;
    } else {
        // Set thread as head and update the rest
        add_thread->lib_thread_next = total_threads;
        total_threads = add_thread;
    }
    return;
}

void lwp_start(void){
    // Converts calling thread into a LWP and yields

    // Create context of thread
    thread start_thread = (thread) malloc(sizeof(context)); // Allocate thread
    if (start_thread == NULL){
        return;
    }

    // Setup context
    start_thread->tid = ++tid_counter;   // Assign tid and increment counter
    start_thread->stack = NULL;
    start_thread->stacksize = 0;
    start_thread->status = MKTERMSTAT(LWP_LIVE, 0);
    start_thread->exited = NULL;

    main_tid = start_thread->tid;  // Update main tid
    curr_thread = start_thread;    // Start thread is current thread

    sched->admit(start_thread);
    add2TotalList(start_thread);
    
    lwp_yield();    // Yield to scheduler
}

tid_t lwp_wait(int *status){
    // Deallocates resources of a terminated thread
    thread oldest_thr;
    tid_t oldest_tid;

    if (exiting_threads != NULL){
        // Thread in exiting list
        curr_thread->exited = exiting_threads;
        removeFromExitList();

    } else if (sched->qlen() <= 1){
        // No processes to block for
        return NO_THREAD;

    } else {
        // Block until a thread terminates
        sched->remove(curr_thread);
        add2WaitList(curr_thread);
        lwp_yield();    // Yield so that other threads may call lwp_exit
    }

    oldest_thr = curr_thread->exited;
    oldest_tid = oldest_thr->tid;
    if (oldest_tid != main_tid){
        // Can deallocate stack
        if (status != NULL){
            *status = oldest_thr->tid;
        }

        removeFromTotalList(oldest_thr);
        int retVal = munmap((void *) oldest_thr->stack, oldest_thr->stacksize);
        if (retVal == -1){
            perror("ERROR: munmap() Failed");
            exit(EXIT_FAILURE);
        }
        free(oldest_thr);
    }

    //MAYBE FREE EVEN IF MAIN ??? 
    return oldest_tid;
}

void add2WaitList(thread add_thr){
    // Add a thread into the waiting thread list
    // Waiting thread list acts as a FIFO
    if (waiting_threads == NULL){
        // Initializing total thread list
        waiting_threads = add_thr;
        add_thr->lib_waiting_next = NULL;

    } else {
        // Add thread to end of queue
        thread thr_ptr = waiting_threads;
        while (thr_ptr->lib_waiting_next != NULL){
            thr_ptr = thr_ptr->lib_waiting_next;    // Increment pointer
        }

        thr_ptr->lib_waiting_next = add_thr;
        add_thr->lib_waiting_next = NULL;
    }
    return;
}

void add2ExitList(thread add_thr){
    // Add a thread into the waiting thread list
    // Waiting thread list acts as a FIFO
    if (exiting_threads == NULL){
        // Initializing total thread list
        exiting_threads = add_thr;
        add_thr->lib_waiting_next = NULL;

    } else {
        // Add thread to end of queue
        thread thr_ptr = exiting_threads;
        while (thr_ptr->lib_waiting_next != NULL){
            thr_ptr = thr_ptr->lib_waiting_next;    // Increment pointer
        }

        thr_ptr->lib_waiting_next = add_thr;
        add_thr->lib_waiting_next = NULL;
    }
    return;
}

void removeFromWaitList(void){
    // Remove the oldest thread (head) from the waiting thread list
    if (waiting_threads == NULL){
        return;
    }

    waiting_threads = waiting_threads->lib_waiting_next;    // Update head
    return;
}

void removeFromExitList(void){
    // Remove the oldest thread (head) from the exiting thread list
    if (exiting_threads == NULL){
        return;
    }

    exiting_threads = exiting_threads->lib_waiting_next;    // Update head
    return;
}

void removeFromTotalList(thread remove_thr){
    // Remove thread from the list of all threads
    if (total_threads == NULL){
        return; // Nothing to remove
    }

    if (total_threads->tid == remove_thr->tid){
        // Removing head
        if (total_threads->lib_thread_next == NULL){
            // Only one thing in list
            total_threads = NULL;
        } else {
            total_threads = total_threads->lib_thread_next;
        }
        return;
    }

    // Removing from somewhere in the middle or end
    thread thr_ptr = total_threads->lib_thread_next;
    thread prev_thr_ptr = total_threads;
    while (thr_ptr != NULL){
        if (thr_ptr->tid == remove_thr->tid){
            prev_thr_ptr->lib_thread_next = thr_ptr->lib_thread_next;
            return;
        }

        prev_thr_ptr = prev_thr_ptr->lib_thread_next;
        thr_ptr = thr_ptr->lib_thread_next;
    }
    return;
}

void lwp_exit(int status){
    // Terminates calling thread and yields to any remaining threads
    if (sched->qlen() == 0 && waiting_threads == NULL){
        // Nothing to exit, but might not hit due to qlen needing time to update
        return;
    }

    if (curr_thread == NULL){
        return; // Simple error checking
    }

    curr_thread->status = MKTERMSTAT(LWP_TERM, status);  // Update thread status
    sched->remove(curr_thread);

    if (waiting_threads != NULL){
        // Thread waiting so let it use this as terminating thread
        thread waiting_threads_head = waiting_threads;
        removeFromWaitList();
        waiting_threads_head->exited = curr_thread;
        sched->admit(waiting_threads_head); // Readmit waiting thread
    } else {
        // No waiting threads
        add2ExitList(curr_thread);
    }

    lwp_yield(); // Thread terminated
    return;
}

void lwp_yield(void){
    // Make a thread yield control to another thread
    thread old_thread = curr_thread;

    curr_thread = sched->next();    // Update current thread
    swap_rfiles(&old_thread->state, &curr_thread->state);   // Swap registers
    return;
}

tid_t lwp_gettid(void){
    // Returns thread id of calling thread
    if (curr_thread == NULL){
        return NO_THREAD;
    }
    return curr_thread->tid;
}

thread tid2thread(tid_t tid){
    // Returns the thread corresponding to the given thread id
    thread thread_ptr = total_threads;
    while (thread_ptr){
        if (thread_ptr->tid == tid && !LWPTERMINATED(thread_ptr->status)){
            // Need tid to match and only looking at non terminated threads
            return thread_ptr;
        }

        thread_ptr = thread_ptr->lib_thread_next;
    }
    return NULL; // Invalid thread id passed in
}

void lwp_set_scheduler(scheduler s){
    if (s == sched){
        return; // Dont need to swap
    }

    scheduler old_s = sched;
    if (s->init != NULL){
        s->init();  // Initialize scheduler if function exists
    }

    // Transfer threads from old to new
    thread thread_ptr;
    while ((thread_ptr = old_s->next()) != NULL){
        old_s->remove(thread_ptr);
        s->admit(thread_ptr);
    }

    if (old_s->shutdown != NULL){
        old_s->shutdown();  // Shutdown scheduler if function exists
    }

    sched = s;  // Update current scheduler
}

scheduler lwp_get_scheduler(void){
    // Returns pointer to current scheduler
    return sched;
}