#pragma once

#include <stdio.h>
#include <tchar.h>
#include <synchapi.h>


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

struct _AsyncQueue
{
	SRWLOCK  mutex;
	PCONDITION_VARIABLE cond;
	Queue queue;
	unsigned int waiting_threads;
	int ref_count;
};

AsyncQueue* async_queue_new(void);
void async_queue_push(AsyncQueue *queue, void *data);
void* async_queue_pop(AsyncQueue *queue);
void* async_queue_try_pop(AsyncQueue *queue);
void* async_queue_timeout_pop(AsyncQueue *queue,
	int     timeout);
int async_queue_length(AsyncQueue *queue);
void async_queue_destroy(AsyncQueue *queue);
