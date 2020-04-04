#pragma once

#include <stdio.h>

#ifdef _WIN32
//Head files that WIN Visual Studio needs
#include <tchar.h>
#include <synchapi.h>

#else
//Head files that linux system needs
//TRUE FALSE BOOL that linux not support
#include <pthread.h>
#define TRUE 1
#define FALSE 0
#define BOOL _Bool

#endif

typedef struct _List List;

struct _List
{
	void* data;
	List *next;
	List *prev;
};

typedef struct _Queue Queue;

struct _Queue
{
	List *head;
	List *tail;
	unsigned int length;
};

typedef struct _AsyncQueue AsyncQueue;

#ifdef _WIN32
//mutex and cond are different in WIN and Linux.
struct _AsyncQueue
{
	SRWLOCK  mutex;
	CONDITION_VARIABLE cond;
	Queue queue;
	unsigned int waiting_threads;
	int ref_count;
};

#else

struct _AsyncQueue
{
	pthread_mutex_t  mutex;
	pthread_cond_t   cond;
	Queue queue;
	unsigned int waiting_threads;
	int ref_count;
};

#endif

// Creat a AsyncQueue. 
// If successed, it return a point to a AsyncQueue. 
// Otherwise, it return a NULL point.
// 创建一个异步队列。如果成功，返回队列的指针，否则，返回NULL。
AsyncQueue* async_queue_new(void);

// Push a point data to the AsyncQueue head. After this, the queue length will increase one.
// input AsyncQueue point; data point.
// 将数据指针推送到异步队形头，然后队列长度增加1。 输入参数： 队列指针，数据指针。
void async_queue_push(AsyncQueue *queue, void *data);

// Pop a point of the data from AsyncQueue tail, After this, the queue length will decrease one and return a point.
// It will waiting point until there is one. It is a blocking function.
// 将一个数据从队列中弹出，然后队列长度减少1，返回数据的指针。 这个函数会一直等到有数据，这是一个阻塞函数。
void* async_queue_pop(AsyncQueue *queue);

// Try Pop a point of the data from AsyncQueue tail, If successed, the queue length will decrease one and return a point.
// If not successed, it return NULL. This function return immediately.
// 尝试将一数据从队列中弹出，如果成功，队列长度减少1，返回数据指针。 如果无数据，就返回NULL，这个函数是立即返回。
void* async_queue_try_pop(AsyncQueue *queue);

// Pop a point of the data from AsyncQueue tail until the timeout. 
// If during the timeout, there is data, the queue length will decrease one and return a point.
// If during the timeout, there is no data, it return NULL.
// the unit of timeout is us.
// 在等待的时间内尝试将一数据从队列中弹出，如果成功，队列长度减少1，返回数据指针。 如果无数据，就返回NULL。
// 这个函数的返回时间是小于等于等待时间的。
void* async_queue_timeout_pop(AsyncQueue *queue,
	long long     timeout);

// return the length of AsyncQueue
// 返回异步队列的长度
int async_queue_length(AsyncQueue *queue);

// release the AsyncQueue
// 释放异步队列资源
void async_queue_destroy(AsyncQueue *queue);
