/*
 * @Author: your name
 * @Date: 2021-07-28 16:17:36
 * @LastEditTime: 2021-08-02 22:46:39
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \old_server_0.1\threadpool.h
 */
#ifndef THREADPOOL
#define THREADPOOL
#include "requestData.h"
#include <pthread.h>

const int THREADPOOL_INVALID = -1;      //线程池无效
const int THREADPOOL_LOCK_FAILURE = -2; //线程池锁失败
const int THREADPOOL_QUEUE_FULL = -3;   //队列已满
const int THREADPOOL_SHUTDOWN = -4;     //关闭状态
const int THREADPOOL_THREAD_FAILURE = -5;//线程失败？？
const int THREADPOOL_GRACEFUL = 1;      //优雅的？？

const int MAX_THREADS = 1024;
const int MAX_QUEUE   = 65535;


typedef enum
{
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

/*
    @var function Pointer to the function that will perform the task
    @指向即将执行任务的函数指针
    @var argument Argument to be passed to the function
    @要传递给函数的参数
*/

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/*
    * @struct threadpool
    * @brief The threadpool struct
    * 
    * @ var lock
    * @ var notify       Condition variable to the notify worker threads    @通知工作线程的条件变量
    * @ var thread_count Number of worker thread ID.                        @工作线程ID数量
    * @ var queue        Array containing the task queue.                   @包含任务队列的数组
    * @ var queue_size   Size of the task queue.                            @任务队列的大小
    * @ var head         Index of the first element.                        @第一个元素的索引
    * @ var tail         Index of the next element.                         @下一个元素的索引
    * @ var count        Number of pending tasks.                           @挂起的任务数
    * @ var shutdown     Flag indicating if the pool is shutting down       @指示池是否正在关闭的标志
    * @ var started      Number of started threads                          @已启动的线程数
*/

struct threadpool_t
{
    pthread_mutex_t lock;
    pthread_cond_t  notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
};


threadpool_t *threadpool_create(int thread_conunt, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);
static void *threadpool_thread(void *threadpool);

#endif