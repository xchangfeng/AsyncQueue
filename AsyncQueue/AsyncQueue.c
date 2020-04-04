
// Copyright: Copyright(c) 2020
// Created on 2020 - 4 - 2
// Author : Changfeng Xia
// Version 1.0
// Title : AsyncQueue.cpp
// Descripition: A Queue for multi-threaded asynchronous data transfering


#include "AsyncQueue.h"
#include <synchapi.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

//队列初始化函数
void queue_init(Queue *queue)
{
	if (queue == NULL)
		return;

	queue->head = queue->tail = NULL;
	queue->length = 0;
}

void queue_push_head(Queue *queue, void *data)
{
	if (queue == NULL)
		return;
	if (data == NULL)
		return;
	List *new_list;
	new_list = (List *)malloc(sizeof(List));
	if (new_list == NULL)
		return;
	new_list->data = data;
	new_list->next = queue->head;
	if (queue->head)
	{
		queue->head->prev = new_list;
	}
	new_list->prev = NULL;
	queue->head = new_list;

	if (!queue->tail)
		queue->tail = queue->head;
	queue->length++;
}

void* queue_pop_tail(Queue *queue)
{
	if (queue == NULL)
		return NULL;

	if (queue->tail)
	{
		List *node = queue->tail;
		void *data = node->data;

		queue->tail = node->prev;
		if (queue->tail)
			queue->tail->next = NULL;
		else
			queue->head = NULL;
		queue->length--;
		free(node);

		return data;
	}

	return NULL;
}

List* queue_peek_tail_link(Queue *queue)
{
	if (queue == NULL)
		return NULL;

	return queue->tail;
}

void queue_clear(Queue *queue)
{
	if (queue == NULL)
		return;

	while (queue->head)
	{
		List *free_list;
		free_list = queue->head;
		queue->head = queue->head->next;
		free(free_list);
	}
	queue_init(queue);
}

//
//异步队列初始化函数，同时开启空间使用。
AsyncQueue* async_queue_new(void)
{
	AsyncQueue *queue;

	queue =(AsyncQueue *) malloc(sizeof(AsyncQueue));
	if (queue == NULL)
		return NULL;

	InitializeSRWLock(&queue->mutex);
	InitializeConditionVariable(&queue->cond);
	queue_init(&queue->queue);
	queue->waiting_threads = 0;
	queue->ref_count = 1;

	return queue;
}


void async_queue_push(AsyncQueue *queue, void *data)
{

	if (queue == NULL)
		return;
	if (data == NULL)
		return;

	AcquireSRWLockExclusive(&queue->mutex);

	queue_push_head (&queue->queue, data);
    if (queue->waiting_threads > 0)
		WakeConditionVariable(&queue->cond);

	ReleaseSRWLockExclusive(&queue->mutex);
}

static void* async_queue_pop_intern_unlocked(AsyncQueue *queue,
	BOOL     wait,
	long       end_time)
{
	void *retval;

	if (!queue_peek_tail_link(&queue->queue) && wait)
	{
		queue->waiting_threads++;
		while (!queue_peek_tail_link(&queue->queue))
		{
			if (end_time == -1)
			    SleepConditionVariableSRW(&queue->cond, &queue->mutex, INFINITE, 0);
			else
			{
				if (!SleepConditionVariableSRW(&queue->cond, &queue->mutex, end_time, 0)) //end_time与持续时间不同
					break;
			}
		}
		queue->waiting_threads--;
	}

	retval = queue_pop_tail(&queue->queue);

	//g_assert(retval || !wait || end_time > 0);

	return retval;
}

void* async_queue_pop(AsyncQueue *queue)
{
	void *retval;

	if (queue == NULL)
		return NULL;

	AcquireSRWLockExclusive(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, TRUE, -1);
	ReleaseSRWLockExclusive(&queue->mutex);

	return retval;
}

void* async_queue_try_pop(AsyncQueue *queue)
{
	void *retval;

	if (queue == NULL)
		return NULL;

	AcquireSRWLockExclusive(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, FALSE, -1);
	ReleaseSRWLockExclusive(&queue->mutex);

	return retval;
}

//timeout单位是us
void* async_queue_timeout_pop(AsyncQueue *queue,
	long      timeout)
{
	//long end_time = g_get_monotonic_time() + timeout;
	void *retval;

	if (queue == NULL)
		return NULL;

	AcquireSRWLockExclusive(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, TRUE, timeout);
	ReleaseSRWLockExclusive(&queue->mutex);

	return retval;
}

int async_queue_length(AsyncQueue *queue)
{
	int retval;

	if (queue == NULL)
		return 0;

	AcquireSRWLockExclusive(&queue->mutex);
	retval = queue->queue.length - queue->waiting_threads;
	ReleaseSRWLockExclusive(&queue->mutex);

	return retval;
}

void async_queue_destroy (AsyncQueue *queue)
{
	if (queue == NULL)
		return ;

    queue_clear(&queue->queue);
    free(queue);
}