/* Vários cliente; 1 servidor; Sem semáforos e mutexes, Queue bounded. */
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
	DWORD maxItens;
	EVENT itensAvailable;
	EVENT itensSpace;
	EVENT accessQueue;
} QUEUE, *PQUEUE;

typedef struct _queue_entry {
	LIST_ENTRY link;
	LONG value;
	LONG answer;
	EVENT haveAnswer;
} QUEUE_ENTRY, *PQUEUE_ENTRY;

QUEUE queue;

VOID QueueInit(PQUEUE pqueue, DWORD maxItens);
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry);
PQUEUE_ENTRY QueueGet(PQUEUE pqueue);
VOID QueueEntryInit(PQUEUE_ENTRY pentry);

VOID QueueInit(PQUEUE pqueue, DWORD maxItens) {
	EventInit(&pqueue->accessQueue, true);
	EventInit(&pqueue->itensAvailable, false);
	EventInit(&pqueue->itensSpace, false);
	InitializeListHead(&pqueue->link);
	pqueue->nItens = 0;
	pqueue->maxItens = maxItens;

}
/* A versão QueuePut1 e QueueGet1 compromete a invariância do sincronizador: lista vazia e evento itensAvailable sinalizado e/ou lista cheia e o evento itensSpace sinalizado. */
VOID QueuePut1(PQUEUE pqueue, PQUEUE_ENTRY pentry) {
	EventWait(&pqueue->accessQueue);
	while (pqueue->nItens == pqueue->maxItens) {
		EventSet(&pqueue->accessQueue);
		EventWait(&pqueue->itensSpace);
		EventWait(&pqueue->accessQueue);
	}
	InsertTailList(&pqueue->link, &pentry->link);
	pqueue->nItens++;
	EventSet(&pqueue->itensAvailable);
	EventSet(&pqueue->accessQueue);
}
PQUEUE_ENTRY QueueGet1(PQUEUE pqueue) {
	PQUEUE_ENTRY pentry;

	EventWait(&pqueue->accessQueue);
	while (pqueue->nItens == 0) {
		EventSet(&pqueue->accessQueue);
		EventWait(&pqueue->itensAvailable);
		EventWait(&pqueue->accessQueue);
	}
	pentry = CONTAINING_RECORD(RemoveHeadList(&pqueue->link), QUEUE_ENTRY, link);
	pqueue->nItens--;
	EventSet(&pqueue->itensSpace);
	EventSet(&pqueue->accessQueue);

	return pentry;
}
/* Versão onde a invariância não é comprometida. */
VOID QueuePut(PQUEUE pqueue, PQUEUE_ENTRY pentry) {
	EventWait(&pqueue->accessQueue);
	while (pqueue->nItens == pqueue->maxItens) {
		EventSet(&pqueue->accessQueue);
		EventWait(&pqueue->itensSpace);
		EventWait(&pqueue->accessQueue);
	}
	InsertTailList(&pqueue->link, &pentry->link);
	pqueue->nItens++;
	if (pqueue->nItens-1 == 0)
		EventSet(&pqueue->itensAvailable);
	EventSet(&pqueue->accessQueue);
}
PQUEUE_ENTRY QueueGet(PQUEUE pqueue) {
	PQUEUE_ENTRY pentry;

	EventWait(&pqueue->accessQueue);
	while (pqueue->nItens == 0) {
		EventSet(&pqueue->accessQueue);
		EventWait(&pqueue->itensAvailable);
		EventWait(&pqueue->accessQueue);
	}
	pentry = CONTAINING_RECORD(RemoveHeadList(&pqueue->link), QUEUE_ENTRY, link);
	pqueue->nItens--;
	if (pqueue->nItens+1 == pqueue->maxItens)
		EventSet(&pqueue->itensSpace);
	EventSet(&pqueue->accessQueue);

	return pentry;
}
VOID QueueEntryInit(PQUEUE_ENTRY pentry) {
	EventInit(&pentry->haveAnswer, false);
}

LONG numbers[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

VOID Client(UT_ARGUMENT arg) {
	DWORD id = (DWORD)arg;
	PQUEUE_ENTRY pentry = (PQUEUE_ENTRY)malloc(sizeof(QUEUE_ENTRY));

	QueueEntryInit(pentry); //

	for (int i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {

		pentry->value = numbers[i];

		QueuePut1(&queue, pentry);
		EventWait(&pentry->haveAnswer);

		printf("Client %d: fact(%d) = %d\n", id, numbers[i], pentry->answer);
	}

	free(pentry);
}

long factorial(long n) {
	long r = n;
	if (n == 0) return 1;
	while (--n > 1)
		r *= n;
	return r;
}

BOOL end = false;

VOID Server(UT_ARGUMENT arg) {
	for (;;) {
		PQUEUE_ENTRY pentry = QueueGet1(&queue);
		if (end)
			break;
		pentry->answer = factorial(pentry->value);
		EventSet(&pentry->haveAnswer);
	}
}

#define MAX_CLIENTS 10

DWORD _tmain(DWORD argc, PTCHAR argv[]) {
	int i;

	UtInit();

	QueueInit(&queue, 20);

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