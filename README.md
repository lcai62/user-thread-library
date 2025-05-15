# user-thread-library

This project was implemented as part of CSC369 at the University of Toronto. 

This repository contains an implementation of a user level thread library in C with preemption, locks, and conditions.


[![C][c-shield]][c-url]
[![POSIX][posix-shield]][posix-url]



## API

### Thread Lifecycle

* `Tid thread_create(void (*fn)(void *), void *parg)`
* `void thread_exit(int exit_code)`
* `Tid thread_yield(Tid want_tid)`
* `int thread_kill(Tid tid)`
* `int thread_wait(Tid tid, int *exit_code)`

### Scheduling Primitives

* `Tid thread_sleep(struct wait_queue *queue)`
* `int thread_wakeup(struct wait_queue *queue, int all)`

### Lock Primitives

* `struct lock *lock_create()`
* `void lock_acquire(struct lock *lock)`
* `void lock_release(struct lock *lock)`
* `void lock_destroy(struct lock *lock)`

### Condition Variables

* `struct cv *cv_create()`
* `void cv_wait(struct cv *cv, struct lock *lock)`
* `void cv_signal(struct cv *cv, struct lock *lock)`
* `void cv_broadcast(struct cv *cv, struct lock *lock)`
* `void cv_destroy(struct cv *cv)`

### Semaphore Primitives
* `struct semaphore *sem_create(int initial_value)`
* `void sem_wait(struct semaphore *sem)`
* `void sem_signal(struct semaphore *sem)`
* `void sem_destroy(struct semaphore *sem)`




[c-shield]: https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white
[c-url]: https://en.wikipedia.org/wiki/C_\(programming_language\)
[posix-shield]: https://img.shields.io/badge/POSIX-00427E?style=for-the-badge
[posix-url]: https://en.wikipedia.org/wiki/POSIX

