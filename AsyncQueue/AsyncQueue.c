
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
// For WIN system rewrite a lot of mutex and cond function to linux-like.
// 重写Win下不少函数，使之与Linux下的形式较为一致，方便后续调用
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

// Wait for the wake during the timeout. the unit of timeout is us.
// 在超时时间内等待其他线程唤醒，timeout的单位是us。
BOOL pthread_cond_wait_timeout_us(CONDITION_VARIABLE *cond, SRWLOCK *mutex, long long timeout)
{
	// The unit of timeout in Win is ms, thus divided by 1000. Win下的时间单位是ms，所以除1000。
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
// Some function that need in Linux system. the init function is rewrite because not easy to fit in Windows
// 重写部分Linux下的调用函数，两个初始化函数因为要多带一参量，与Win不同，所以也重写了。
#include <sys/time.h>
int pthread_mutex_init_n(pthread_mutex_t *mutex)
{
	return pthread_mutex_init(mutex, NULL);
}

int pthread_cond_init_n(pthread_cond_t  *cond)
{
	return pthread_cond_init(cond,NULL);
}

// Wait for the wake during the timeout. the unit of timeout is us.
// In linux, The funtion of timedwait is absolute time. So, By add timeout to the time of now to get the until time.
// The unit of timeout is us.
// Notice the return is Negate of the origin value.
// 在超时时间内等待其他线程唤醒，timeout的单位是us。
// 在Linux系统下，这个等待是一个绝对时间，所以需要把超时时间加到当前时间上来。
// 注意返回值在原来函数的返回值的基础上取反
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

#endif

// Initialization of the queue
// 队列初始化函数
void queue_init(Queue *queue)
{
	if (queue == NULL)
		return;

	queue->head = queue->tail = NULL;
	queue->length = 0;
}

// Push a data point to the queue head，and increase the leagth for 1.
// 将一个数据指针推送到队列头，并将队列长度增加1。
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

// Pop a data From the tail of the queue and return the point, If there isn't a data, Then return NUll.
// 从队列尾推出一个数据，并返回这个数据指针。如果没有数据，则返回NULL。
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

// Check If there is data or not in the queue.
// 查看队列是否有数据 
List* queue_peek_tail_link(Queue *queue)
{
	if (queue == NULL)
		return NULL;

	return queue->tail;
}

// Release all the queue, but not the data point. 
// If the data point is generate by malloc, it should free first.
// 释放队列，但不是队列中data指向的数据，如果队列的data是开辟的空间，应先释放。
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

//creat a AsyncQueue and Initialization of mutex and cond
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

// Push a point data to the AsyncQueue head. After this, the queue length will increase one.
// input AsyncQueue point; data point.
// 将数据指针推送到异步队形头，然后队列长度增加1。 输入参数： 队列指针，数据指针。
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

// The inter function of pop, include pop, trypop, timeoutpop.
// BOOL wait，for TRUE is wait, used in pop and timeoutpop. FALSE is used in trypop.
// timeout is wating time, -1 in pop and try pop, Other is time in us for timeoutpop.
// pop使用的内部函数，包括pop, trypop, timeoutpop.
// wait量，当是TRUE的时候，用于pop和timeoutpop；当是FALSE的时候，用于trypop。
// timeout是等待时间，当是pop和trypop时用-1，否则用于等待时间，单位是us。
static void* async_queue_pop_intern_unlocked(AsyncQueue *queue,
	BOOL     wait,
	long long       timeout)
{
	void *retval;

	if (!queue_peek_tail_link(&queue->queue) && wait)
	{
		queue->waiting_threads++;
		while (!queue_peek_tail_link(&queue->queue))
		{
			if (timeout == -1)
			    pthread_cond_wait(&queue->cond, &queue->mutex);
			else
			{
				if (!pthread_cond_wait_timeout_us(&queue->cond, &queue->mutex, timeout))
					break;
			}
		}
		queue->waiting_threads--;
	}

	retval = queue_pop_tail(&queue->queue);

	//g_assert(retval || !wait || end_time > 0);

	return retval;
}

// Pop a point of the data from AsyncQueue tail, After this, the queue length will decrease one and return a point.
// It will waiting point until there is one. It is a blocking function.
// 将一个数据从队列中弹出，然后队列长度减少1，返回数据的指针。 这个函数会一直等到有数据，这是一个阻塞函数。
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

// Try Pop a point of the data from AsyncQueue tail, If successed, the queue length will decrease one and return a point.
// If not successed, it return NULL. This function return immediately.
// 尝试将一数据从队列中弹出，如果成功，队列长度减少1，返回数据指针。 如果无数据，就返回NULL，这个函数是立即返回。
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

// Pop a point of the data from AsyncQueue tail until the timeout. 
// If during the timeout, there is data, the queue length will decrease one and return a point.
// If during the timeout, there is no data, it return NULL.
// the unit of timeout is us.
// 在等待的时间内尝试将一数据从队列中弹出，如果成功，队列长度减少1，返回数据指针。 如果无数据，就返回NULL。
// 这个函数的返回时间是小于等于等待时间的。
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

// return the length of AsyncQueue
// 返回异步队列的长度
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

// release the AsyncQueue
// 释放异步队列资源
void async_queue_destroy (AsyncQueue *queue)
{
	if (queue == NULL)
		return ;
#ifndef _WIN32
	pthread_mutex_destroy(&queue->mutex);
	pthread_cond_destroy(&queue->cond);
#endif
    queue_clear(&queue->queue);
    free(queue);
}
