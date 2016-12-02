/* 1 cliente; 1 servidor; Espera passiva recorrendo ao mecanismo de sincronização Event */
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "UThread.h"
#include "List.h"


#include "USynch.h"

long n, res;
HANDLE utServer, utClient;
BOOL end = false;

#define MAX_WORK	10


EVENT hasAnswer, hasWork;

void Client(UT_ARGUMENT arg) {
	int i;
	long * numbers = (long *)arg;
	for (i = 0; i < MAX_WORK; i++) {
		n = numbers[i];
		EventSet(&hasWork);
		EventWait(&hasAnswer);
		numbers[i] = res;
	}

	end = true;
	EventSet(&hasWork);
}

long factorial (long n) {
	long r = n;
	if (n == 0) return 1;
	while (--n > 1) 
		r *= n;
	return r;
}

void Server(UT_ARGUMENT arg) {
	for (;;) {
		EventWait(&hasWork);
		if (end) 
			break;
		res = factorial(n);
		EventSet(&hasAnswer);
	}
}

DWORD _tmain( DWORD argc, PTCHAR argv[] ) {
	long numbers[MAX_WORK] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	UtInit();

	utClient = UtCreate(Client, numbers);
	utServer = UtCreate(Server, NULL);

	EventInit(&hasAnswer, false);
	EventInit(&hasWork, false);

	UtRun();

	UtEnd();

	printf("Factoriais:\n\t");
	for (int i = 0; i < MAX_WORK; i++)
		printf("%d ", numbers[i]);

	putchar('\n');

	printf("Press enter key to finish...\n");
	getchar();

	return 0;
}