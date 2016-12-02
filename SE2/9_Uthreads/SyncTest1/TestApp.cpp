#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "UThread.h"

long n, res;
bool hasWork = 0, hasAnswer = 0;

#define MAX_WORK	10

void Client(UT_ARGUMENT arg) {
   int i;
   long * numbers = (long *)arg;
   for (i = 0; i < MAX_WORK; i++) {
      n = numbers[i];
      hasWork = true;
      hasAnswer = false;
      while (!hasAnswer) UtYield();
      numbers[i] = res;
   }
}

long factorial (long n) {
	long r = n;
	if (n == 0) return 1;
	while (--n > 1) 
		r *= n;
	return r;
}

void Server(UT_ARGUMENT arg) {
   for (int i = 0; i < MAX_WORK; i++) {
      while (!hasWork) UtYield();
      res = factorial(n);
      hasAnswer = true;
      hasWork = false;
   }
}


DWORD _tmain( DWORD argc, PTCHAR argv[] ) {
	long numbers[MAX_WORK] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	UtInit();

	HANDLE client = UtCreate(Client, numbers);
	HANDLE server = UtCreate(Server, NULL);

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