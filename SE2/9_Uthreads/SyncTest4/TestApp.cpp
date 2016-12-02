/* Vários cliente; 1 servidor; Sem semáforos e mutexes, Queue unbounded. */
#include <malloc.h>
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "USynch.h"
#include "UThread.h"
#include "List.h"


typedef struct _queue {
	LIST_ENTRY link;
	DWORD nItens;
	EVENT itensAvailable;
	EVENT accessQueue;
} QUEUE, *PQUEUE;

typedef struct _queue_entry {
	LIST_ENTRY link;
	LONG value;
	LONG answer;
	EVENT haveAnswer;
} QUEUE_ENTRY, * PQUEUE_ENTRY;

QUEUE queue;

VOID QueueInit(PQUEUE pqueue);
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry);
PQUEUE_ENTRY QueueGet(PQUEUE pqueue);
VOID QueueEntryInit(PQUEUE_ENTRY pentry);

VOID QueueInit(PQUEUE pqueue) {
	EventInit(&pqueue->accessQueue, true);
	EventInit(&pqueue->itensAvailable, false);
	InitializeListHead(&pqueue->link);
	pqueue->nItens = 0;
}
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry) {
	EventWait(&pqueue->accessQueue);
	InsertTailList(&pqueue->link, &pentry->link);
	pqueue->nItens++;
	EventSet(&pqueue->itensAvailable);
	EventSet(&pqueue->accessQueue);
}
PQUEUE_ENTRY QueueGet(PQUEUE pqueue) {
	PQUEUE_ENTRY pentry;

	EventWait(&pqueue->accessQueue);

	while ( IsListEmpty(&pqueue->link) ) {
		// Espera por um elemento na queue, mas antes liberta o lock (sinaliza o evento accessItens), caso contrário deadlock
		EventSet(&pqueue->accessQueue);
		EventWait(&pqueue->itensAvailable);
		EventWait(&pqueue->accessQueue);
	}

	pentry = CONTAINING_RECORD(RemoveHeadList(&pqueue->link), QUEUE_ENTRY, link);
	pqueue->nItens--;
	EventSet(&pqueue->accessQueue);

	return pentry;
}

VOID QueueEntryInit(PQUEUE_ENTRY pentry) {
	EventInit(&pentry->haveAnswer, false);
}

LONG numbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

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