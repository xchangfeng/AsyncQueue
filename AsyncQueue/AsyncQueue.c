
// Copyright: Copyright(c) 2020
// Created on 2020 - 4 - 2
// Author : Changfeng Xia
// Version 1.0
// Title : AsyncQueue.cpp
// Descripition: A Queue for multi-threaded asynchronous data transfering


#include "AsyncQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#ifdef _WIN32

#include <windows.h>

void pthread_mutex_init_n(SRWLOCK *SRWLock)
{
  InitializeSRWLock(SRWLock);
}

void pthread_mutex_lock(SRWLOCK *SRWLock)
{
	AcquireSRWLockExclusive(SRWLock);
}

void pthread_mutex_unlock(SRWLOCK *SRWLock)
{
	ReleaseSRWLockExclusive(SRWLock);
}

BOOL pthread_cond_wait(CONDITION_VARIABLE *cond, SRWLOCK *mutex)
{
	return SleepConditionVariableSRW(cond, mutex, INFINITE, 0);
}

BOOL pthread_cond_wait_timeout_us(CONDITION_VARIABLE *cond, SRWLOCK *mutex, long long timeout)
{
	int timewait = timeout / 1000;
	return SleepConditionVariableSRW(cond, mutex, timewait, 0);
}

void pthread_cond_signal(CONDITION_VARIABLE *cond)
{
	WakeConditionVariable(cond);
}

void pthread_cond_init_n(CONDITION_VARIABLE *cond)
{
	InitializeConditionVariable(cond);
}

#else

#include <sys/time.h>
int pthread_mutex_init_n(pthread_mutex_t *mutex)
{
	return pthread_mutex_init(mutex, NULL);
}

int pthread_cond_wait_timeout_us(pthread_cond_t *cond, pthread_mutex_t *mutex, long long timeout)
{
	struct timespec ts;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	tv.tv_usec += timeout;
	tv.tv_sec += tv.tv_usec / 1000000;
	tv.tv_usec = tv.tv_usec % 1000000;

    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;

    int retval;
	retval =  pthread_cond_timedwait(cond, mutex, &ts);
    
	return !retval;
}

int pthread_cond_init_n(pthread_cond_t  *cond)
{
	return pthread_cond_init(cond,NULL);
}

#endif

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

	pthread_mutex_init_n(&queue->mutex);
	pthread_cond_init_n(&queue->cond);
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

	pthread_mutex_lock(&queue->mutex);

	queue_push_head (&queue->queue, data);
    if (queue->waiting_threads > 0)
		pthread_cond_signal(&queue->cond);

	pthread_mutex_unlock(&queue->mutex);
}

static void* async_queue_pop_intern_unlocked(AsyncQueue *queue,
	BOOL     wait,
	long long       end_time)
{
	void *retval;

	if (!queue_peek_tail_link(&queue->queue) && wait)
	{
		queue->waiting_threads++;
		while (!queue_peek_tail_link(&queue->queue))
		{
			if (end_time == -1)
			    pthread_cond_wait(&queue->cond, &queue->mutex);
			else
			{
				if (!pthread_cond_wait_timeout_us(&queue->cond, &queue->mutex, end_time))
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

	pthread_mutex_lock(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, TRUE, -1);
	pthread_mutex_unlock(&queue->mutex);

	return retval;
}

void* async_queue_try_pop(AsyncQueue *queue)
{
	void *retval;

	if (queue == NULL)
		return NULL;

	pthread_mutex_lock(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, FALSE, -1);
	pthread_mutex_unlock(&queue->mutex);

	return retval;
}

//timeout单位是ms
void* async_queue_timeout_pop(AsyncQueue *queue,
	long long    timeout)
{
	//long end_time = g_get_monotonic_time() + timeout;
	void *retval;

	if (queue == NULL)
		return NULL;

	pthread_mutex_lock(&queue->mutex);
	retval = async_queue_pop_intern_unlocked(queue, TRUE, timeout);
	pthread_mutex_unlock(&queue->mutex);

	return retval;
}

int async_queue_length(AsyncQueue *queue)
{
	int retval;

	if (queue == NULL)
		return 0;

	pthread_mutex_lock(&queue->mutex);
	retval = queue->queue.length - queue->waiting_threads;
	pthread_mutex_unlock(&queue->mutex);

	return retval;
}

void async_queue_destroy (AsyncQueue *queue)
{
	if (queue == NULL)
		return ;

    queue_clear(&queue->queue);
    free(queue);
}