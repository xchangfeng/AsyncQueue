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
	PSRWLOCK mutex;
	PCONDITION_VARIABLE cond;
	Queue queue;
	unsigned int waiting_threads;
	int ref_count;
};