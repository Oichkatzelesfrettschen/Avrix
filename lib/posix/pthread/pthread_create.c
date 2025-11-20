/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file pthread_create.c
 * @brief Thread creation and management
 *
 * Implements pthread_create(), pthread_join(), pthread_exit(), etc.
 * Wraps kernel scheduler task management functions.
 */

#include "pthread.h"
#include "arch/common/hal.h"

extern int errno;

/* Forward declarations of kernel functions (will be implemented in Phase 4) */
extern bool nk_task_create(hal_context_t *ctx, void (*entry)(void), uint8_t prio, void *stack, size_t stack_len);
extern void nk_task_exit(int status) __attribute__((noreturn));
extern uint8_t nk_current_tid(void);
extern void nk_yield(void);

/* Thread wrapper structure */
typedef struct {
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    uint8_t joinable;
} pthread_info_t;

/* Thread info table (one per task) */
static pthread_info_t thread_info[PTHREAD_THREADS_MAX];

/* Thread entry wrapper */
static void pthread_entry_wrapper(void) {
    pthread_t self = pthread_self();
    pthread_info_t *info = &thread_info[self];

    /* Call user's start routine */
    info->retval = info->start_routine(info->arg);

    /* Exit thread */
    pthread_exit(info->retval);
}

/**
 * @brief Create a new thread
 */
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg) {
    if (!thread || !start_routine) {
        return EINVAL;
    }

    /* Default attributes */
    uint8_t detachstate = PTHREAD_CREATE_JOINABLE;
    size_t stacksize = 256;  /* Default stack size */
    void *stackaddr = NULL;

    /* Parse attributes if provided */
    if (attr) {
        detachstate = attr->detachstate;
        stacksize = attr->stacksize ? attr->stacksize : 256;
        stackaddr = attr->stackaddr;
    }

    /* Allocate stack if not provided */
    static uint8_t thread_stacks[PTHREAD_THREADS_MAX][256] __attribute__((section(".noinit")));
    static uint8_t next_thread = 0;

    if (!stackaddr) {
        if (next_thread >= PTHREAD_THREADS_MAX) {
            return EAGAIN;  /* Too many threads */
        }
        stackaddr = thread_stacks[next_thread];
        next_thread++;
    }

    /* Find free slot in thread info table */
    pthread_t tid = next_thread - 1;

    /* Initialize thread info */
    thread_info[tid].start_routine = start_routine;
    thread_info[tid].arg = arg;
    thread_info[tid].retval = NULL;
    thread_info[tid].joinable = (detachstate == PTHREAD_CREATE_JOINABLE);

    /* Create kernel task */
    hal_context_t ctx;
    uint8_t prio = attr ? attr->priority : 128;  /* Default mid-priority */

    if (!nk_task_create(&ctx, pthread_entry_wrapper, prio, stackaddr, stacksize)) {
        return EAGAIN;  /* Task creation failed */
    }

    *thread = tid;
    return 0;
}

/**
 * @brief Wait for thread termination
 */
int pthread_join(pthread_t thread, void **retval) {
    if (thread >= PTHREAD_THREADS_MAX) {
        return EINVAL;
    }

    pthread_info_t *info = &thread_info[thread];

    if (!info->joinable) {
        return EINVAL;  /* Thread is detached */
    }

    /* Wait for thread to exit (busy-wait for now, will be improved) */
    /* TODO: Implement proper wait queue in kernel */
    while (info->start_routine != NULL) {
        nk_yield();
    }

    if (retval) {
        *retval = info->retval;
    }

    return 0;
}

/**
 * @brief Detach a thread
 */
int pthread_detach(pthread_t thread) {
    if (thread >= PTHREAD_THREADS_MAX) {
        return EINVAL;
    }

    thread_info[thread].joinable = 0;
    return 0;
}

/**
 * @brief Terminate calling thread
 */
void pthread_exit(void *retval) {
    pthread_t self = pthread_self();
    pthread_info_t *info = &thread_info[self];

    info->retval = retval;
    info->start_routine = NULL;  /* Mark as exited */

    /* Exit kernel task */
    nk_task_exit(0);
}

/**
 * @brief Get calling thread's ID
 */
pthread_t pthread_self(void) {
    return (pthread_t)nk_current_tid();
}

/**
 * @brief Compare thread IDs
 */
int pthread_equal(pthread_t t1, pthread_t t2) {
    return (t1 == t2) ? 1 : 0;
}

/**
 * @brief Yield processor
 */
int pthread_yield(void) {
    nk_yield();
    return 0;
}

int sched_yield(void) {
    return pthread_yield();
}

/*═══════════════════════════════════════════════════════════════════
 * THREAD ATTRIBUTES
 *═══════════════════════════════════════════════════════════════════*/

int pthread_attr_init(pthread_attr_t *attr) {
    if (!attr) return EINVAL;

    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    attr->priority = 128;
    attr->stacksize = 256;
    attr->stackaddr = NULL;

    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    (void)attr;
    return 0;  /* Nothing to clean up */
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    if (!attr) return EINVAL;
    if (detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED) {
        return EINVAL;
    }

    attr->detachstate = (uint8_t)detachstate;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    if (!attr || !detachstate) return EINVAL;

    *detachstate = attr->detachstate;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if (!attr || stacksize < 64) return EINVAL;

    attr->stacksize = stacksize;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
    if (!attr || !stacksize) return EINVAL;

    *stacksize = attr->stacksize;
    return 0;
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr) {
    if (!attr) return EINVAL;

    attr->stackaddr = stackaddr;
    return 0;
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr) {
    if (!attr || !stackaddr) return EINVAL;

    *stackaddr = attr->stackaddr;
    return 0;
}
