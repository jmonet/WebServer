/*
 * @Author: your name
 * @Date: 2021-07-28 16:17:45
 * @LastEditTime: 2021-08-02 23:11:16
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\threadpool.cpp
 */


#include "threadpool.h"

threadpool_t *threadpool_create(int thread_conunt, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;

    do
    {
        if(thread_conunt <= 0 || thread_conunt > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
        {
            return NULL;
        }

        if( (pool = (threadpool_t *)malloc(sizeof(threadpool_t)) == NULL)
        {
            break;
        }
        
        /*Initialize Variable*/
        pool->thread_count = 0;
        pool->queue_size = queue_size;
        pool->head = pool->tail = pool->count = 0;
        pool->shutdown = pool->started = 0;

        /* Allocate thread and task queue */
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count );
        pool->queue   = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size );

        /* Initialize mutex and conditional variable first */
        if( (pthread_mutex_init(&(pool->lock), NULL) != 0)  ||
            (pthread_cond_init(&(pool->notify), NULL) != 0) ||
            (pool->threads == NULL) ||
            (pool->queue == NULL) )
        {
                break;
        }

        /* Start worker threads */
        for(i = 0; i < thread_count; i++) {
            if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool ) != 0)
            {
                threadpool_destroy(pool, 0);
                return NULL;
            }
            pool->thread_count++;
            pool->started++;
        }
        
        return pool;

    } while (false);
    
    if(pool != NULL)
    {
        threadpool_free(pool);
    }
    return NULL;
}