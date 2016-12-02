/* 1 cliente; 1 servidor; Espera passiva recorrendo apenas aos mecanismos da UtThread */
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "UThread.h"

long n, res;
bool hasWork = false, hasAnswer = false;
HANDLE utServer, utClient;

#define MAX_WORK	10



long factorial (long n) {
	long r = n;
	if (n == 0) return 1;
	while (--n > 1) 
		r *= n;
	return r;
}

BOOL serverStarted = false, clientStarted = false;
BOOL end = false;

void Client(UT_ARGUMENT arg) {
	int i;
	long * numbers = (long *)arg;

	clientStarted = true;
	if ( ! serverStarted ) UtDeactivate();
	
	for (i = 0; i < MAX_WORK; i++) {
		n = numbers[i];
		hasWork = true;
		hasAnswer = false; 
		UtActivate(utServer);
		if (!hasAnswer) UtDeactivate(); 
		numbers[i] = res;
	}

	end = true;
	hasWork = true;
	UtActivate(utServer);

}

void Server(UT_ARGUMENT arg) {
	serverStarted = true;
	if ( clientStarted )
		UtActivate(utClient);

	for (;;) {
		if (!hasWork) UtDeactivate(); 
		if (end)
			break;
		res = factorial(n);
		hasAnswer = true;
		hasWork = false; 
		UtActivate(utClient);
	}
}

DWORD _tmain( DWORD argc, PTCHAR argv[] ) {
	long numbers[MAX_WORK] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	UtInit();

	utClient = UtCreate(Client, numbers);
	utServer = UtCreate(Server, numbers);
	
	UtRun();

	UtEnd();

	printf("factoriais:\n\t");
	for (int i = 0; i < MAX_WORK; i++)
		printf("%d ", numbers[i]);

	putchar('\n');

	printf("Press enter key to finish...\n");
	getchar();

	return 0;
}