#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THREADPOOL_SIZE 2
#define TASK_QUEUE_SIZE 5

typedef struct task_node{
    void (*func)(void *arg);
    void *arg;
    struct task_node *next;
} task_node_t;

typedef struct{
    task_node_t *front;
    task_node_t *rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} task_queue_t;

typedef struct{
    pthread_t threads[THREADPOOL_SIZE];
    task_queue_t queue;
    int stop;
} threadpool_t;

void task_queue_init(task_queue_t *q);
void task_queue_push(task_queue_t *q, void (*func)(void *), void *arg);
int task_queue_pop(task_queue_t *q, void(**func)(void *), void **arg);
void *threadpool_worker(void *arg);
void threadpool_init(threadpool_t *pool);
void threadpool_add(threadpool_t *pool, void(*func)(void *), void *arg);
void threadpool_destroy(threadpool_t *pool);

#ifdef __cplusplus
}
#endif

#endif
