#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"



/* For Assignment 1, you will need a queue structure to keep track of the
 * runnable threads. You can use the tutorial 1 queue implementation if you
 * like. You will probably find in Assignment 2 that the operations needed
 * for the wait_queue are the same as those needed for the ready_queue.
 */

struct thread;

enum Status {
    UNUSED,
    RUNNING,
    RUNNABLE,
    EXITING,
    SLEEPING
};

typedef struct wait_queue_item {
    struct wait_queue_item *next;
    struct wait_queue_item *prev;
    struct thread *item;
} wait_node_t ;

typedef struct wait_queue {
    wait_node_t *head;
    wait_node_t *tail;
} wait_queue_t;


struct wait_queue *wait_queue_create() {
    wait_queue_t *wait_queue = malloc(sizeof(wait_queue_t));
    wait_queue->head = NULL;
    wait_queue->tail = NULL;
    return wait_queue;
}

void wait_queue_destroy(struct wait_queue *wq) {
    free(wq);
}

int wait_queue_enqueue(struct thread *item, wait_queue_t *queue) {
    wait_node_t *new_node = malloc(sizeof(wait_node_t));

    new_node->item = item;
    new_node->next = NULL;
    new_node->prev = NULL;

    if (queue->head == NULL) {
        queue->head = new_node;
        queue->tail = new_node;
    }
    else {
        queue->tail->next = new_node;
        new_node->prev = queue->tail;
        queue->tail = new_node;
    }
    return 0;
}

int wait_queue_dequeue(void **item, wait_queue_t *queue) {
    *item = queue->head->item;

    queue->head = queue->head->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    return 0;
}

void wait_print_queue(wait_queue_t *queue)
{
    /* empty check */
    if (queue->head == NULL) {
        printf("(nil)\n");
        return;
    }

    wait_node_t *curr = queue->head;

    while (curr != NULL) {
        fprintf(stderr, "%p -> ", curr->item);

        curr = curr->next;
    }

    fprintf(stderr, "(nil)\n");
}

/* This is the thread control block. */
struct thread {
    ucontext_t *context;
    char *stack;
    Tid tid;
    enum Status status;
    int stack_allocated;
    int killed;
    wait_queue_t *wait_queue;
    wait_queue_t *sleeping_in;
    int exit_code;
    int exit_code_valid;
};

struct thread *thread_table[THREAD_MAX_THREADS];

typedef struct queue_node {
    struct queue_node* next;
    struct queue_node* prev;
    struct thread *item;
} queue_node_t;

/* ERROR CODES */
#define ERR_FULL            -1  /* The maximum capacity is reached. */
#define ERR_INVALID_ARG     -2  /* Invalid pointer is passed to dequeue. */
#define ERR_EMPTY           -3  /* The queue is empty when dequeue is called. */
#define ERR_NO_SUCH_ITEM    -4  /* The item to be removed is not in the queue. */
#define ERR_NOT_INITIALIZED -5  /* Initialization function is not called yet. */
#define ERR_INITIALIZED     -6  /* Try to initialize when already initialized. */
#define ERR_NOT_IMPLEMENTED -9  /* function not implemented yet */







bool queue_initialized = false;

queue_node_t *queue_head = NULL;
queue_node_t *queue_tail = NULL;

queue_node_t *exiting_head = NULL;
queue_node_t *exiting_tail = NULL;


int exiting_enqueue(struct thread *item)
{
    /* create the new node */
    queue_node_t *new_node = malloc(sizeof(queue_node_t));
    if (new_node == NULL) {
        perror("malloc");
        exit(1);
    }
    new_node->item = item;
    new_node->next = NULL;
    new_node->prev = NULL;

    /* check if queue is empty */
    if (exiting_tail == NULL) {
        exiting_head = new_node;
        exiting_tail = new_node;
    }
        /* items in queue, set pointers */
    else {
        exiting_tail->next = new_node;
        new_node->prev = exiting_tail;
        exiting_tail = new_node;
    }
    return 0;
}

int exiting_dequeue(void **item)
{
    /* initialization check */
    if (queue_initialized == false) return ERR_NOT_INITIALIZED;

    /* empty check */
    if (exiting_head == NULL) return ERR_EMPTY;

    /* invalid pointer check */
    if (item == NULL) return ERR_INVALID_ARG;

    *item = exiting_head->item;
    queue_node_t *node = exiting_head;
    node = node;

    exiting_head = exiting_head->next;

    /* last item in queue */
    if (exiting_head == NULL) {
        exiting_tail = NULL;
    }
//    free369(node);

    return 0;

}

void exiting_clean() {

    if (exiting_head == NULL) {
        return;
    }

    queue_node_t *curr = exiting_head;

    while (curr != NULL) {
        if (curr->item->exit_code_valid == false) {
            /* nothing waiting for the exit code */
            curr->item->status = UNUSED;
            curr = curr->next;
        }
        else {
            curr = curr->next;
        }

    }

    exiting_head = NULL;
    exiting_tail = NULL;

}

void exiting_print_queue()
{

    /* initialization check */
    if (queue_initialized == false) {
        return;
    }

    /* empty check */
    if (exiting_head == NULL) {
        printf("(nil)\n");
        return;
    }

    queue_node_t *curr = exiting_head;

    while (curr != NULL) {
        fprintf(stderr, "[%d: %d] -> ", curr->item->tid, curr->item->status);

        curr = curr->next;
    }

    exiting_head->prev = NULL;
    exiting_tail->next = NULL;

    fprintf(stderr, "(nil)\n");
}




/* Perform any initialization needed so that the queue data structure can be
 * used.
 * Returns 0 on success or an error if the queue has already been initialized.
 */
int queue_initialize()
{
    if (queue_initialized) {
        return ERR_INITIALIZED;
    }
    queue_head = NULL;
    queue_tail = NULL;
    queue_initialized = true;
    return 0;
}

/* Add item to the tail of the queue.
 * Returns 0 on success or an appropriate error code on failure.
 */
int queue_enqueue(struct thread *item)
{
    /* initialization check */
    if (queue_initialized == false) return ERR_NOT_INITIALIZED;

    /* create the new node */
    queue_node_t *new_node = malloc(sizeof(queue_node_t));
    if (new_node == NULL) {
        perror("malloc");
        exit(1);
    }
    new_node->item = item;
    new_node->next = NULL;
    new_node->prev = NULL;

    /* check if queue is empty */
    if (queue_tail == NULL) {
        queue_head = new_node;
        queue_tail = new_node;
    }
        /* items in queue, set pointers */
    else {
        queue_tail->next = new_node;
        new_node->prev = queue_tail;
        queue_tail = new_node;
    }
    return 0;
}

/* Remove the item at the head of the queue and store it in the location
 * pointed to by 'item'.
 * Returns 0 on success or an appropriate error code on failure.
 */
int queue_dequeue(void **item)
{
    /* initialization check */
    if (queue_initialized == false) return ERR_NOT_INITIALIZED;

    /* empty check */
    if (queue_head == NULL) return ERR_EMPTY;

    /* invalid pointer check */
    if (item == NULL) return ERR_INVALID_ARG;

    *item = queue_head->item;
    queue_node_t *node = queue_head;

    queue_head = queue_head->next;

    /* last item in queue */
    if (queue_head == NULL) {
        queue_tail = NULL;
    }
    node = node;

    return 0;

}

/* Print the contents of the queue from head to tail.
 * Do not change the existing printf's, outside of the Add BEGIN/END markers.
 * Refer to tutorial handout for expected output format.
 */
void queue_print_queue()
{

    /* initialization check */
    if (queue_initialized == false) {
        return;
    }

    /* empty check */
    if (queue_head == NULL) {
        printf("(nil)\n");
        return;
    }

    queue_node_t *curr = queue_head;

    while (curr != NULL) {
        fprintf(stderr, "[%d: %d] -> ", curr->item->tid, curr->item->status);

        curr = curr->next;
    }

    queue_head->prev = NULL;
    queue_tail->next = NULL;

    fprintf(stderr, "(nil)\n");
}


/* Search the queue for 'item' and, if found, remove it from the queue.
 * Returns 0 if the item is found, or an error code if the item is not
 * in the queue.
 */
int queue_remove_tid_from_queue(Tid tid)
{

    /* initialization check */
    if (queue_initialized == false) return ERR_NOT_INITIALIZED;

    queue_node_t *curr = queue_head;

    while (curr != NULL) {
        if (curr->item->tid == tid) {
            break;
        }
        curr = curr->next;
    }

    /* item does not exist after traversing whole queue */
    if (!curr) {
        return ERR_NO_SUCH_ITEM;
    }

    /* item is the head */
    if (curr->prev == NULL) {

        /* item is the only item */
        if (curr->next == NULL) {
            queue_head = NULL;
            queue_tail = NULL;
        }
        else {
            queue_node_t *temp = curr;
            queue_head = curr->next;
            queue_head->prev = NULL;
            temp->next = NULL;
        }


    }
        /* item is the tail */
    else if (curr->next == NULL) {

        /* item is the only item */
        if (curr->prev == NULL) {
            queue_head = NULL;
            queue_tail = NULL;
        }
        else {
            queue_node_t *temp = curr;
            queue_tail = curr->prev;
            queue_tail->next = NULL;
            temp->prev = NULL;
        }

    }
    else {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
    }

//    free369(curr);

    return 0;

}



/* Remove any items remaining in the queue and free any dynamically allocated
 * memory used by the queue for these items, restoring the queue to the fresh,
 * uninitialized state.
 * Returns 0 on success, or an error if the queue has not been initialized.
 */
int queue_destroy()
{
    /* initialization check */
    if (queue_initialized == false) return ERR_NOT_INITIALIZED;

    /* traverse and free */
    queue_node_t *curr = queue_head;
    while (curr != NULL) {
//        queue_node_t *temp = curr;
        curr = curr->next;
//        free369(temp);
    }
    queue_initialized = false;
    return 0;

}

/**************************************************************************
 * Assignment 1: Global Variables
 **************************************************************************/


Tid current_thread;


/**************************************************************************
 * Assignment 1: Refer to thread.h for the detailed descriptions of the six
 *               functions you need to implement.
 **************************************************************************/

void
thread_init(void)
{
    interrupts_set(false);
    /* initializing thread table to all unused */
    for (int i = 0; i < THREAD_MAX_THREADS; i++) {
        thread_table[i] = malloc(sizeof(struct thread));
        thread_table[i]->context = malloc(sizeof(ucontext_t));
        thread_table[i]->status = UNUSED;
    }

    /* initializing the ready queue */
    queue_initialize();

    /* set current thread */
    current_thread = 0;

    /* set init thread to running */
    thread_table[current_thread]->status = RUNNING;
    thread_table[current_thread]->wait_queue = wait_queue_create();

    interrupts_set(true);
}

Tid
thread_id()
{
    return current_thread;
}

int
thread_status(Tid tid)
{
    return thread_table[tid]->status;
}

/* New thread starts by calling thread_stub. The arguments to thread_stub are
 * the thread_main() function, and one argument to the thread_main() function.
 */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
    interrupts_on();
    bool enabled = interrupts_enabled();
    assert(enabled);
    thread_main(arg); // call thread_main() function with arg
    thread_exit(0);
}


Tid find_free_tid() {
    for (int i = 0 ; i < THREAD_MAX_THREADS; i++) {
        if (thread_table[i]->status == UNUSED) {
            return i;
        }
    }
    return -1;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    int enabled = interrupts_set(false);
    /* find space for a thread */
    int tid = find_free_tid();

    if (tid == -1) {
        interrupts_set(enabled);
        return THREAD_NOMORE;
    }

    /* allocate space for the stack */
    void *new_stack = malloc(THREAD_MIN_STACK);
    if (new_stack == NULL) {
        interrupts_set(enabled);
        return THREAD_NOMEMORY;
    }

    /* retrieve and modify new context */
    ucontext_t *new_context = malloc(sizeof(ucontext_t));

    getcontext(new_context);



    /* adding the new stack */
    new_context->uc_stack.ss_sp = new_stack;
    new_context->uc_stack.ss_size = THREAD_MIN_STACK;


    /* changing the RIP, RDI, RSI, RSP */
    new_context->uc_mcontext.gregs[REG_RIP] = (greg_t) thread_stub; /* instruction pointer */
    new_context->uc_mcontext.gregs[REG_RDI] = (greg_t) fn; /* first arg */
    new_context->uc_mcontext.gregs[REG_RSI] = (greg_t) parg; /* second arg */
    new_context->uc_mcontext.gregs[REG_RSP] = (greg_t) (new_stack + THREAD_MIN_STACK - 8); /* stack pointer */
    /* (need to waste 8 bytes ) */


    /* create the new thread control block */
    struct thread *new_thread = thread_table[tid];
    new_thread->status = RUNNABLE;
    new_thread->tid = tid;
    new_thread->context = new_context;
    new_thread->killed = false;
    new_thread->stack = new_stack;
    new_thread->stack_allocated = 0;

    wait_queue_t *wait_queue = wait_queue_create();
    new_thread->wait_queue = wait_queue;

    new_thread->sleeping_in = NULL;
    new_thread->exit_code = 0;
    new_thread->exit_code_valid = false;

    /* update the global */
    thread_table[tid] = new_thread;

    /* add the new thread to the queue */
    queue_enqueue(new_thread);


    interrupts_set(enabled);
    return tid;
}

Tid
thread_yield(Tid want_tid) {

    int enabled = interrupts_set(false);

    if (thread_table[current_thread]->status == EXITING) {
        struct thread *want_thread;

        queue_dequeue((void **) &want_thread);

        /* set the global currently running thread */
        current_thread = want_thread->tid;


        /* cleanup any threads before switching */
        if (exiting_head != NULL) {
            exiting_clean();
        }

        /* set the next element of the thread queue to running */
        want_thread->status = RUNNING;


//        unintr_printf("switching from thread %d to thread %d\n", before, current_thread);
        interrupts_off();
        interrupts_set(enabled);
        /* context switch */

        setcontext(want_thread->context);

    }

    volatile int already_switched = 0;

    /**************************************************************************
        Saving context
    **************************************************************************/

    getcontext((thread_table[current_thread]->context));

    if (already_switched) {
        // check if current thread was been killed
        if (thread_table[current_thread]->killed == true) {
            thread_table[current_thread]->status = EXITING;

            thread_exit(1);
        }

        /* don't do a context switch */
//        unintr_printf("not doing a context switch, staying in thread %d\n", current_thread);
        interrupts_set(enabled);
//        unintr_printf("interrupts set back\n");

        return want_tid;
    }
    already_switched = 1;



    /**************************************************************************
        Error checking
    **************************************************************************/
    if (want_tid < -2 || want_tid >= THREAD_MAX_THREADS) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }

    if (want_tid >= 0) {
        if (thread_table[want_tid]->status == UNUSED) {
            interrupts_set(enabled);
            return THREAD_INVALID;
        }
    }

    if (queue_head == NULL && want_tid == THREAD_ANY) {
        interrupts_set(enabled);
        return THREAD_NONE;
    }


    /**************************************************************************
        Modify the current thread
    **************************************************************************/
    /* set current thread to ready */
    if (thread_table[current_thread]->status != SLEEPING) {
        thread_table[current_thread]->status = RUNNABLE;
    }

    /* add current thread to queue */
    if (thread_table[current_thread]->status != SLEEPING) {
        queue_enqueue((thread_table[current_thread]));
    }





    /**************************************************************************
        Fetch and modify the target thread
    **************************************************************************/
    /* get the want thread */
    struct thread *want_thread;

    if (want_tid == THREAD_ANY) {

        queue_dequeue((void **) &want_thread);
        want_tid = want_thread->tid;
    }
        /* look for specific thread */
    else {

        if (want_tid == THREAD_SELF) {
            want_tid = current_thread;
        }

        want_thread = (thread_table[want_tid]);
        queue_remove_tid_from_queue(want_tid);

    }


    /* set the global currently running thread */
    current_thread = want_thread->tid;


    /* cleanup any threads before switching */
    if (exiting_head != NULL) {
        exiting_clean();
    }

    /* set the next element of the thread queue to running */
    want_thread->status = RUNNING;

    /* context switch */
//    unintr_printf("current thread %d\n", current_thread);
//    unintr_printf("switching from thread %d to thread %d\n", before, current_thread);

    setcontext(want_thread->context);

    return want_tid;
}

void
thread_exit(int exit_code)
{
    int enabled = interrupts_set(false);


    /* mark thread as exiting */
    thread_table[current_thread]->status = EXITING;
    thread_table[current_thread]->exit_code = exit_code;
    thread_table[current_thread]->exit_code_valid = true;
    thread_wakeup(thread_table[current_thread]->wait_queue, true);

    /* add thread to LL of exiting threads */
    exiting_enqueue(thread_table[current_thread]);


    /* exit if no threads in ready queue */
    if (queue_head == NULL) {
        interrupts_set(enabled);
        exit(exit_code);
    }

    interrupts_set(enabled);
    thread_yield(THREAD_ANY);

    /* should not reach here */
    return;
}

Tid
thread_kill(Tid tid)
{
    int enabled = interrupts_set(false);
    if (tid >= THREAD_MAX_THREADS) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }

    if (tid == current_thread || thread_table[tid]->status != RUNNABLE) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    /* check if sleeping */
    if (thread_table[tid]->status == SLEEPING) {
        struct thread *thread;
        wait_queue_dequeue((void **)&thread, thread_table[tid]->sleeping_in);
        queue_enqueue(thread_table[tid]);
    }


    /* mark the thread as killed */
    thread_table[tid]->killed = true;
    thread_table[tid]->exit_code = -SIGKILL;
    thread_table[tid]->exit_code_valid = true;

    interrupts_set(enabled);
    return tid;
}

/**************************************************************************
 * Important: The rest of the code should be implemented in Assignment 2. *
 **************************************************************************/


Tid
thread_sleep(struct wait_queue *queue)
{

    int enabled = interrupts_set(false);

    if (queue == NULL) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    if (queue_head == NULL) {
        interrupts_set(enabled);
        return THREAD_NONE;
    }

    thread_table[current_thread]->status = SLEEPING;
    wait_queue_enqueue(thread_table[current_thread], queue);
    thread_table[current_thread]->sleeping_in = queue;



    int yielded_to = thread_yield(THREAD_ANY);

    interrupts_set(enabled);
    return yielded_to;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all) {

    int enabled = interrupts_set(false);

    if (queue == NULL) {
        interrupts_set(enabled);
        return 0;
    }
    if (queue->head == NULL) {
        interrupts_set(enabled);
        return 0;
    }

    if (all == 0) {
        /* wake up one thread */
        struct thread *want_thread;
        wait_queue_dequeue((void **) &want_thread, queue);
        want_thread->status = RUNNABLE;
        want_thread->sleeping_in = NULL;
        queue_enqueue(want_thread);

        interrupts_set(enabled);
        return 1;
    }
    else {
        int count = 0;

        while (queue->head != NULL) {
            struct thread *want_thread;
            wait_queue_dequeue((void **) &want_thread, queue);
            want_thread->status = RUNNABLE;
            want_thread->sleeping_in = NULL;
            queue_enqueue(want_thread);
            count++;
        }

        interrupts_set(enabled);
        return count;
    }
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid, int *exit_code)
{
    int enabled = interrupts_set(false);
    if (tid < 0 || tid >= THREAD_MAX_THREADS) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    if (thread_table[tid]->status == UNUSED) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    if (tid == current_thread) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    if (thread_table[tid]->wait_queue->head != NULL) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }
    if (thread_table[tid]->killed == true) {
        interrupts_set(enabled);
        return THREAD_INVALID;
    }

    thread_sleep(thread_table[tid]->wait_queue);

    /* thread has been killed */
    if (exit_code != NULL) {
        *exit_code = thread_table[tid]->exit_code;
        thread_table[tid]->exit_code_valid = false;
    }

    interrupts_set(enabled);
    return tid;

}
enum LOCK_STATUS {
    LOCKED,
    UNLOCKED
};

struct lock {
    Tid owner;
    bool status;
    wait_queue_t *queue;
};

struct lock *
lock_create()
{
    int enabled = interrupts_set(false);


    struct lock *lock;

    lock = malloc(sizeof(struct lock));
    assert(lock);

    lock->status = UNLOCKED;
    lock->queue = wait_queue_create();
    lock->owner = -1;

    interrupts_set(enabled);
    return lock;
}

void
lock_destroy(struct lock *lock)
{
    int enabled = interrupts_set(false);

    assert(lock != NULL);

    if (lock->status == LOCKED) {
        interrupts_set(enabled);
        return;
    }

//    free369(lock->queue);
//    free369(lock);

    interrupts_set(enabled);
}

void
lock_acquire(struct lock *lock)
{
    int enabled = interrupts_set(false);

    assert(lock != NULL);

    while (lock->status == LOCKED) {
        thread_sleep(lock->queue);
    }
    lock->status = LOCKED;
    lock->owner = current_thread;

    interrupts_set(enabled);

}

void
lock_release(struct lock *lock)
{
    int enabled = interrupts_set(false);

    assert(lock != NULL);

    if (lock->owner != current_thread) {
        interrupts_set(enabled);
        return;
    }

    lock->status = UNLOCKED;
    lock->owner = -1;
    thread_wakeup(lock->queue, true);

    interrupts_set(enabled);
}

struct cv {
    wait_queue_t *queue;
};

struct cv *
cv_create()
{
    int enabled = interrupts_set(false);
    struct cv *cv;

    cv = malloc(sizeof(struct cv));
    assert(cv);

    cv->queue = wait_queue_create();

    interrupts_set(enabled);
    return cv;
}

void
cv_destroy(struct cv *cv)
{
    int enabled = interrupts_set(false);
    assert(cv != NULL);

    wait_queue_destroy(cv->queue);

//    free(cv);
    interrupts_set(enabled);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(false);
    assert(cv != NULL);
    assert(lock != NULL);

    lock_release(lock);
    thread_sleep(cv->queue);
    lock_acquire(lock);

    interrupts_set(enabled);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(false);
    assert(cv != NULL);
    assert(lock != NULL);

    thread_wakeup(cv->queue, false);

    interrupts_set(enabled);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
    int enabled = interrupts_set(false);
    assert(cv != NULL);
    assert(lock != NULL);

    thread_wakeup(cv->queue, true);

    interrupts_set(enabled);
}

struct semaphore {
    int value;
    wait_queue_t *queue;
};

struct semaphore *sem_create(int initial_value) {
    int enabled = interrupts_set(false);
    struct semaphore *sem = malloc(sizeof(struct semaphore));
    assert(sem);

    sem->value = initial_value;
    sem->queue = wait_queue_create();
    interrupts_set(enabled);
    return sem;
}

void sem_wait(struct semaphore *sem) {
    int enabled = interrupts_set(false);
    assert(sem);

    sem->value--;

    if (sem->value < 0) {
        thread_sleep(sem->queue);
    }

    interrupts_set(enabled);
}


void sem_destroy(struct semaphore *sem) {
    int enabled = interrupts_set(false);
    assert(sem);
    wait_queue_destroy(sem->queue);
    // free369(sem); // if you're tracking frees
    interrupts_set(enabled);
}

void sem_signal(struct semaphore *sem) {
    int enabled = interrupts_set(false);
    assert(sem);

    sem->value++;

    if (sem->value <= 0) {
        thread_wakeup(sem->queue, false);
    }

    interrupts_set(enabled);
}

