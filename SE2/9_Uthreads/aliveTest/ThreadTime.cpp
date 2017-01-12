#include "UThread.h"
#include <Windows.h>
#include <stdio.h>

HANDLE thrd, thrd1, thrd2;
DWORD time = 0;

static void thrd_1(UT_ARGUMENT arg) {
	DWORD time = GetTickCount();
	printf("thread 1 started!\n");
	int count = 0;
	for (int i = 0;i < 1000;++i) {
		for (int j = 0;j < 1000;++j) {
			++count;
		}
	}
	printf("thread 1 ended!\n");
	DWORD thrTime = (GetTickCount() - time);
	printf("thread took %d miliseconds\n", thrTime);
	UtYield();
}

static void thrd_2(UT_ARGUMENT arg) {
	DWORD time = GetTickCount();
	printf("thread 2 started!\n");
	int count = 0;
	for (int i = 0; i < 100000000;++i) {
		++count;
	}
	printf("thread 2 ended!\n");
	DWORD thrTime = (GetTickCount() - time);
	printf("thread took %d miliseconds\n", thrTime);
	UtYield();
}

static void thrd_3(UT_ARGUMENT arg) {
	DWORD time = GetTickCount();
	printf("thread 3 started!\n");
	int count = 0;
	for (int i = 0;i < 10000000;++i) {
		++count;
	}
	printf("thread 3 ended!\n");
	DWORD thrTime = (GetTickCount() - time);
	printf("thread took %d miliseconds\n", thrTime);
	UtYield();
}

int main(int argc, char **argv) {
	
	UtInit();

	thrd = UtCreate(thrd_1, "t1");
	thrd1 = UtCreate(thrd_2, "t2");
	thrd2 = UtCreate(thrd_3, "t3");
	UtRun();

	UtEnd();
	printf("Click to continue...\n");
	getchar();
	return 0;
}