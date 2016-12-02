#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "UThread.h"
#include "List.h"


#include "USynch.h"

#define MAX_WORK	10
DWORD sharedVar;
EVENT accessEvt;
UTHREAD_MUTEX accessMux;


VOID MyThread1(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		tmp = sharedVar;
		if ((rand() % 10) == 0)
			UtYield();
		tmp += 1;
		sharedVar = tmp;
	}
}

VOID f1() {

	EventWait(&accessEvt);
	//...
	EventSet(&accessEvt);

}
VOID MyThread2(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		EventWait(&accessEvt);
		tmp = sharedVar;
		if ((rand() % 10) == 0)
			UtYield();
		tmp += 1;
		//f1();
		sharedVar = tmp;
		EventSet(&accessEvt);
		if ((rand() % 10) == 0)
			UtYield();
	}
}

VOID f2() {

	UtAcquireMutex(&accessMux);
	//...
	UtReleaseMutex(&accessMux);

}
VOID MyThread3(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		UtAcquireMutex(&accessMux);
		tmp = sharedVar;
		if ((rand() % 10) == 0)
			UtYield();
		tmp += 1;
		f2();
		sharedVar = tmp;
		UtReleaseMutex(&accessMux);
		if ((rand() % 10) == 0)
			UtYield();
	}
}


VOID test1() {
	sharedVar = 0;
	EventInit(&accessEvt, true);

	UtCreate(MyThread1, NULL);
	UtCreate(MyThread1, NULL);
	UtCreate(MyThread1, NULL);

	UtRun();

	UtEnd();
	
	printf("Test1: sharedVar = %d\n", sharedVar);

	printf("Press enter key to finish...\n");
	getchar();
}

VOID test2() {
	sharedVar = 0;
	EventInit(&accessEvt, true);

	UtCreate(MyThread2, NULL);
	UtCreate(MyThread2, NULL);
	UtCreate(MyThread2, NULL);

	UtRun();

	UtEnd();

	printf("Test2: sharedVar = %d\n", sharedVar);

	printf("Press enter key to finish...\n");
	getchar();
}

VOID test3() {
	sharedVar = 0;
	UtInitializeMutex(&accessMux, false);

	UtCreate(MyThread3, NULL);
	UtCreate(MyThread3, NULL);
	UtCreate(MyThread3, NULL);

	UtRun();

	UtEnd();

	printf("Test3: sharedVar = %d\n", sharedVar);

	printf("Press enter key to finish...\n");
	getchar();
}

DWORD _tmain(DWORD argc, PTCHAR argv[]) {
	srand(GetTickCount());
	UtInit();

	test1();
	test2();
	test3();

	return 0;
}