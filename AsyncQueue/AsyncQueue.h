#pragma once

#include <stdio.h>

#ifdef _WIN32

#include <tchar.h>
#include <synchapi.h>

#else

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

AsyncQueue* async_queue_new(void);
void async_queue_push(AsyncQueue *queue, void *data);
void* async_queue_pop(AsyncQueue *queue);
void* async_queue_try_pop(AsyncQueue *queue);
void* async_queue_timeout_pop(AsyncQueue *queue,
	long long     timeout);
int async_queue_length(AsyncQueue *queue);
void async_queue_destroy(AsyncQueue *queue);
