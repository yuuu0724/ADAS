#include <threadpool.h>

extern int system_exit;  // 由主程序控制退出

// ----------------- 任务队列 -----------------
void task_queue_init(task_queue_t *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void task_queue_push(task_queue_t *q, void (*func)(void *), void *arg)
{
    pthread_mutex_lock(&q->mutex);

    if (q->size >= TASK_QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }

    task_node_t *node = (task_node_t *)malloc(sizeof(task_node_t));
    node->func = func;
    node->arg = arg;
    node->next = NULL;

    if (!q->rear) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int task_queue_pop(task_queue_t *q, void (**func)(void *), void **arg)
{
    pthread_mutex_lock(&q->mutex);
    while (!q->front && !system_exit) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (!q->front) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    task_node_t *node = q->front;
    *func = node->func;
    *arg = node->arg;

    q->front = node->next;
    if (!q->front) q->rear = NULL;

    q->size--;
    free(node);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

// ----------------- 线程池工作函数 -----------------
void *threadpool_worker(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;

    while (!pool->stop) {
        void (*func)(void *) = NULL;
        void *task_arg = NULL;
        if (task_queue_pop(&pool->queue, &func, &task_arg) == 0) {
            func(task_arg);
        }
    }
    return NULL;
}

// ----------------- 初始化线程池 -----------------
void threadpool_init(threadpool_t *pool)
{
    pool->stop = 0;
    task_queue_init(&pool->queue);

    for (int i = 0; i < THREADPOOL_SIZE; i++) {
        pthread_create(&pool->threads[i], NULL, threadpool_worker, pool);
    }
}

// ----------------- 添加任务 -----------------
void threadpool_add(threadpool_t *pool, void (*func)(void *), void *arg)
{
    task_queue_push(&pool->queue, func, arg);
}

// ----------------- 销毁线程池 -----------------
void threadpool_destroy(threadpool_t *pool)
{
    pool->stop = 1;
    pthread_cond_broadcast(&pool->queue.cond);

    for (int i = 0; i < THREADPOOL_SIZE; i++) {
        pthread_join(pool->threads[i], NULL);
    }
}
