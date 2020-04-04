
#include "AsyncQueue.h"
#include <malloc.h>


#ifdef _WIN32

DWORD WINAPI produce(LPVOID lpParameter)
{
	AsyncQueue* pThreadData = (AsyncQueue*)lpParameter;

	for (int i = 0; i <100; ++i)
	{
		
		if (i % 3 == 1)
			Sleep(3000);
		else
			Sleep(1000);
		
		int *producedata;
		producedata = (int *)malloc(1*sizeof(int));
		producedata[0] = i;
		//printf_s("pruduce before the queue length is %d\n", async_queue_length(pThreadData));
		async_queue_push(pThreadData, producedata);
		printf_s("pruduce cycle time is %d and the number is %d\n", i, *producedata);
		printf_s("pruduce end the queue length is %d\n", async_queue_length(pThreadData));
	}
	return 0;
}

DWORD WINAPI comsumer(LPVOID lpParameter)
{
	AsyncQueue* pThreadData = (AsyncQueue*)lpParameter;
	//printf_s("now is comsumer.%x\n", pThreadData);
	for (int i = 0; i <100; ++i)
	{
		Sleep(1000);
		int *comsumerdata;
		comsumerdata = async_queue_pop(pThreadData);

		// comsumerdata = async_queue_try_pop(pThreadData);

		// comsumerdata = async_queue_timeout_pop(pThreadData,3000000);

		if (comsumerdata != NULL)
		{
			printf_s("comsumer cycle time is %d and the number is %d\n", i, *comsumerdata);
			free(comsumerdata);
		}
	}
	return 0;
}

int main()
{
	AsyncQueue *queue;
	queue = async_queue_new();

	HANDLE hThread1 = CreateThread(NULL, 0, produce, queue, 0, NULL);
	
	HANDLE hThread2 = CreateThread(NULL, 0, comsumer, queue, 0, NULL);

	getchar();
	return 0;
}

#else
#include <pthread.h>
#include <unistd.h>

void* produce(void* lpParameter)
{
	AsyncQueue* pThreadData = (AsyncQueue*)lpParameter;

	for (int i = 0; i <100; ++i)
	{
		
		if (i % 3 == 1)
			sleep(3);
		else
			sleep(1);
		
		int *producedata;
		producedata = (int *)malloc(1*sizeof(int));
		producedata[0] = i;
		//printf_s("pruduce before the queue length is %d\n", async_queue_length(pThreadData));
		async_queue_push(pThreadData, producedata);
		printf("pruduce cycle time is %d and the number is %d\n", i, *producedata);
		printf("pruduce end the queue length is %d\n", async_queue_length(pThreadData));
	}
	return 0;
}

void* comsumer(void* lpParameter)
{
	AsyncQueue* pThreadData = (AsyncQueue*)lpParameter;
	//printf_s("now is comsumer.%x\n", pThreadData);
	for (int i = 0; i <100; ++i)
	{
		sleep(1);
		int *comsumerdata;
		comsumerdata = async_queue_pop(pThreadData);

		// comsumerdata = async_queue_try_pop(pThreadData);

		// comsumerdata = async_queue_timeout_pop(pThreadData,5000000);
        // printf("comsumerdata is %x\n", comsumerdata);
		if (comsumerdata != NULL)
		{
			printf("comsumer cycle time is %d and the number is %d\n", i, *comsumerdata);
			free(comsumerdata);
		}
	}
	return 0;
}

int main()
{
	AsyncQueue *queue;
	queue = async_queue_new();

    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, produce, queue);
    pthread_create(&tid2, NULL, comsumer, queue);

	getchar();
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	return 0;
}

#endif