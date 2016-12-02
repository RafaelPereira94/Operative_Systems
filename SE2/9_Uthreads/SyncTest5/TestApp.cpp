/* Vários cliente; 1 servidor; Com semáforos e mutexes */
#include <malloc.h>
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "USynch.h"
#include "UThread.h"
#include "List.h"


typedef struct _queue {
	LIST_ENTRY link;
	SEMAPHORE itensAvailable;
	SEMAPHORE itensSpace;
	UTHREAD_MUTEX accessQueue;
} QUEUE, *PQUEUE;

typedef struct _queue_entry {
	LIST_ENTRY link;
	LONG value;
	LONG answer;
	EVENT haveAnswer;
} QUEUE_ENTRY, * PQUEUE_ENTRY;

QUEUE queue;
#define MAX_QUEUE_ENTRIES	100

VOID QueueInit(PQUEUE pqueue);
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry);
PQUEUE_ENTRY QueueGet(PQUEUE pqueue);
VOID QueueEntryInit(PQUEUE_ENTRY pentry);

VOID QueueInit(PQUEUE pqueue) {
	UtInitializeMutex(&pqueue->accessQueue, false);
	SemaphoreInit(&pqueue->itensAvailable, 0, MAX_QUEUE_ENTRIES);
	SemaphoreInit(&pqueue->itensSpace, MAX_QUEUE_ENTRIES, MAX_QUEUE_ENTRIES);
	InitializeListHead(&pqueue->link);
}
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry) {
	SemaphoreAcquire(&pqueue->itensSpace, 1);
	UtAcquireMutex(&pqueue->accessQueue);
	InsertTailList(&pqueue->link, &pentry->link);
	SemaphoreRelease(&pqueue->itensAvailable, 1);
	UtReleaseMutex(&pqueue->accessQueue);
}
PQUEUE_ENTRY QueueGet(PQUEUE pqueue) {
	PQUEUE_ENTRY pentry;

	SemaphoreAcquire(&pqueue->itensAvailable, 1);
	UtAcquireMutex(&pqueue->accessQueue);
	pentry = CONTAINING_RECORD(RemoveHeadList(&pqueue->link), QUEUE_ENTRY, link);
	SemaphoreRelease(&pqueue->itensSpace, 1);
	UtReleaseMutex(&pqueue->accessQueue);

	return pentry;
}

VOID QueueEntryInit(PQUEUE_ENTRY pentry) {
	EventInit(&pentry->haveAnswer, false);
}

LONG numbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
UTHREAD_COUNTER_LATCH clientsCounter;

VOID Client(UT_ARGUMENT arg) {
	DWORD id = (DWORD)arg;
	PQUEUE_ENTRY pentry = (PQUEUE_ENTRY)malloc(sizeof(QUEUE_ENTRY));

	QueueEntryInit(pentry); //

	for(int i = 0; i < sizeof(numbers)/sizeof(numbers[0]); i++) {
		
		pentry->value = numbers[i];

		QueuePut(&queue, pentry);
		EventWait(&pentry->haveAnswer);

		printf("Client %d: fact(%d) = %d\n", id, numbers[i], pentry->answer);
	}

	free(pentry);

	UtSignalCounterLatch(&clientsCounter);
}

long factorial (long n) {
	long r = n;
	if (n == 0) return 1;
	while (--n > 1) 
		r *= n;
	return r;
}

BOOL end = false;

VOID Server(UT_ARGUMENT arg) {
	for (;;) {
		PQUEUE_ENTRY pentry = QueueGet(&queue);
		if (end)
			break;
		pentry->answer = factorial(pentry->value);
		EventSet(&pentry->haveAnswer);
	}
	printf("Server: finishing\n");
}

QUEUE_ENTRY dummy;
VOID Manager(UT_ARGUMENT arg) {
	UtWaitCounterLatch(&clientsCounter);
	printf("Manager: finishing\n");
	end = true;
	QueuePut(&queue, &dummy);
}

#define MAX_CLIENTS 10

DWORD _tmain( DWORD argc, PTCHAR argv[] ) {
	int i;

	UtInit();

	UtInitCounterLatch(&clientsCounter, MAX_CLIENTS);

	QueueInit(&queue);

	for (i = 0; i < MAX_CLIENTS; i++) {
		UtCreate(Client, (UT_ARGUMENT)i);
	}
	UtCreate(Server, NULL);
	UtCreate(Manager, NULL);

	UtRun();

	UtEnd();

	printf("Press enter key to finish...\n");
	getchar();

	return 0;
}
