#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "UThread.h"
#include "List.h"


#include "USynch.h"

#define MAX_WORK	10
DWORD sharedVar = 0;
EVENT acquireAccessEvt;
UTHREAD_MUTEX acquireAccessMux;


VOID MyThread1(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		tmp = sharedVar;
		tmp += 1;
		if ((rand() % 4) == 0)
			UtYield();
		sharedVar = tmp;
	}
}

VOID f1() {
	DWORD tmp;
	EventWait(&acquireAccessEvt);
	tmp = sharedVar;
	tmp += 1;
	if ((rand() % 4) == 0)
		UtYield();
	sharedVar = tmp;
	EventSet(&acquireAccessEvt);
}

VOID MyThread2(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		EventWait(&acquireAccessEvt);
		tmp = sharedVar;
		tmp += 1;
		if ((rand() % 4) == 0)
			UtYield();
//		f1();
		sharedVar = tmp;
		EventSet(&acquireAccessEvt);
	}
}

VOID f2() {
	DWORD tmp;
	UtAcquireMutex(&acquireAccessMux);
	tmp = sharedVar;
	tmp += 1;
	if ((rand() % 4) == 0)
		UtYield();
	sharedVar = tmp;
	UtReleaseMutex(&acquireAccessMux);
}

VOID MyThread3(UT_ARGUMENT arg) {
	DWORD i = 0, tmp;
	for (i = 0; i < MAX_WORK; i++) {
		UtAcquireMutex(&acquireAccessMux);
		tmp = sharedVar;
		tmp += 1;
		if ((rand() % 4) == 0)
			UtYield();
		sharedVar = tmp;
		f2();
		UtReleaseMutex(&acquireAccessMux);
	}
}

VOID test3() {
	sharedVar = 0;
	UtInitializeMutex(&acquireAccessMux, false);

	UtCreate(MyThread3, NULL);
	UtCreate(MyThread3, NULL);
	UtCreate(MyThread3, NULL);

	UtRun();

	UtEnd();

	printf("Test3: sharedVar = %d\n", sharedVar);

	printf("Press enter key to finish...\n");
	getchar();
}


VOID test2() {
	sharedVar = 0;
	EventInit(&acquireAccessEvt, true);

	UtCreate(MyThread2, NULL);
	UtCreate(MyThread2, NULL);
	UtCreate(MyThread2, NULL);

	UtRun();

	UtEnd();

	printf("Test2: sharedVar = %d\n", sharedVar);

	printf("Press enter key to finish...\n");
	getchar();
}


VOID test1() {
	sharedVar = 0;

	UtCreate(MyThread1, NULL);
	UtCreate(MyThread1, NULL);
	UtCreate(MyThread1, NULL);

	UtRun();

	UtEnd();
	
	printf("Test1: sharedVar = %d\n", sharedVar);

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