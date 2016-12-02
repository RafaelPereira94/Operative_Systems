/* Vários cliente; 1 servidor; Sem semáforos e com mutexes, Queue unbounded. Não está tratado a conclusão do servidor. */
#include <malloc.h>
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "USynch.h"
#include "UThread.h"
#include "List.h"


typedef struct _queue {
	LIST_ENTRY link;
	EVENT itensAvailable;
	UTHREAD_MUTEX accessMux;
} QUEUE, *PQUEUE;

typedef struct _queue_entry {
	LIST_ENTRY link;
	LONG value;
	LONG answer;
	EVENT hasAnswer;
} QUEUE_ENTRY, * PQUEUE_ENTRY;

static QUEUE queue;

VOID QueueInit(PQUEUE pqueue);
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry);
PQUEUE_ENTRY QueueGet(PQUEUE pqueue);
VOID QueueEntryInit(PQUEUE_ENTRY pentry);

VOID QueueInit(PQUEUE pqueue) {
	InitializeListHead(&pqueue->link);
	EventInit(&pqueue->itensAvailable, false);
	UtInitializeMutex(&pqueue->accessMux, false);
}
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry) {
	//QueueEntryInit(pentry);
	UtAcquireMutex(&pqueue->accessMux);
	InsertTailList(&pqueue->link, &pentry->link);
	EventSet(&pqueue->itensAvailable);
	UtReleaseMutex(&pqueue->accessMux);
}
PQUEUE_ENTRY QueueGet(PQUEUE pqueue) {
	PQUEUE_ENTRY pentry;

	UtAcquireMutex(&pqueue->accessMux);

	while (IsListEmpty(&pqueue->link)) {
		UtReleaseMutex(&pqueue->accessMux);
		EventWait(&pqueue->itensAvailable);
		UtAcquireMutex(&pqueue->accessMux);
	}

	pentry = CONTAINING_RECORD(RemoveHeadList(&pqueue->link), QUEUE_ENTRY, link);

	UtReleaseMutex(&pqueue->accessMux);
	return pentry;
}

VOID QueueEntryInit(PQUEUE_ENTRY pentry) {
	EventInit(&pentry->hasAnswer, false);
}

LONG numbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

VOID Client(UT_ARGUMENT arg) {
	DWORD id = (DWORD)arg;
	PQUEUE_ENTRY pentry = (PQUEUE_ENTRY)malloc(sizeof(QUEUE_ENTRY));

	QueueEntryInit(pentry); //

	for(int i = 0; i < sizeof(numbers)/sizeof(numbers[0]); i++) {
		
		pentry->value = numbers[i];

		QueuePut(&queue, pentry);

		EventWait(&pentry->hasAnswer);

		printf("Client %d: fact(%d) = %d\n", id, numbers[i], pentry->answer);
	}

	free(pentry);
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

		EventSet(&pentry->hasAnswer);
	}
}

#define MAX_CLIENTS 10

DWORD _tmain( DWORD argc, PTCHAR argv[] ) {
	int i;

	UtInit();

	QueueInit(&queue);

	for (i = 0; i < MAX_CLIENTS; i++) {
		UtCreate(Client, (UT_ARGUMENT)i);
	}
	UtCreate(Server, NULL);

	UtRun();

	UtEnd();

	printf("Press enter key to finish...\n");
	getchar();

	return 0;
}