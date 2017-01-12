#include "UThread.h"
#include <stdio.h>

HANDLE th1;

static void thread_1(UT_ARGUMENT arg) {
	printf("Thread1 start!\n");
	printf("Thread t1 is -> %s\n", UtThreadState(th1) == 0 ? "ready" : UtThreadState(th1) == 1 ? "run" : "block");
	printf("Thread1 end!\n");
}


/*int main(int argc, char **argv) {
	UtInit();

	th1 = UtCreate(thread_1, "t1");
	printf("thread1 created!\n");

	printf("Thread t1 is -> %s\n", UtThreadState(th1) == 0 ? "ready" : UtThreadState(th1) == 1 ? "run" : "block");

	UtRun();

	printf("Thread t1 is -> %s\n", UtThreadState(th1) == 0 ? "ready" : UtThreadState(th1) == 1 ? "run" : "block");

	UtEnd();
	getchar();

	return 0;
}*/