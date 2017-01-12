#include <stdio.h>
#include "UThread.h"

HANDLE thread1, thread2;

static void thread_1(UT_ARGUMENT arg) {
	printf("Thread1 start!\n");
	printf("Thread1 end!\n");
}

static void thread_2(UT_ARGUMENT arg) {
	printf("Thread2 start!\n");
	UtYield();
	printf("Thread2 end!\n");
}

/*int main(int argc, char **argv) {

	UtInit();

	thread1 = UtCreate(thread_1, "t1");
	printf("thread1 created!\n");
	thread2 = UtCreate(thread_2, "t2");
	printf("thread2 created!\n");

	printf("Thread t1 is -> %d\n", UtAlive(thread1));
	printf("thread t2 is -> %d\n", UtAlive(thread2));

	printf("Running threads...\n");
	UtRun();

	printf("Thread t1 is -> %d\n", UtAlive(thread1));
	printf("thread t2 is -> %d\n", UtAlive(thread2));
	
	UtEnd();

	printf("Click to continue...\n");
	getchar();
	return 0;

}*/