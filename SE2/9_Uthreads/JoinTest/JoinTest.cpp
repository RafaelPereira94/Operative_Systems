#include <stdio.h>
#include "List.h"
#include "..\Include\Uthread.h"


///////////////////////////////////////////////////////////////
//															 //
// Test 1: N threads, each one printing its number M times //
//															 //
///////////////////////////////////////////////////////////////


HANDLE tA, tB, tC;

VOID ThreadA(UT_ARGUMENT Argument) {
	UCHAR Char = (UCHAR)Argument;

	printf("Thread %c start\n", Char);
	UtJoin(tB);
	printf("Thread %c end\n", Char);
}
VOID ThreadB(UT_ARGUMENT Argument) {
	UCHAR Char = (UCHAR)Argument;

	printf("Thread %c start\n", Char);
	printf("Thread %c end\n", Char);
}
VOID ThreadC(UT_ARGUMENT Argument) {
	UCHAR Char = (UCHAR)Argument;

	printf("Thread %c start\n", Char);
	UtJoin(tB);
	printf("Thread %c end\n", Char);
}

VOID Test1()  {
	printf("\n :: Test 1 - BEGIN :: \n\n");

	tA = UtCreate(ThreadA, (UT_ARGUMENT)'A');
	tB = UtCreate(ThreadB, (UT_ARGUMENT)'B');
	tC = UtCreate(ThreadC, (UT_ARGUMENT)'C');

	UtRun();

	printf("\n\n :: Test 1 - END :: \n");
}

int main() {
	UtInit();

	Test1();

	printf("Press any key to finish");
	getchar();


	UtEnd();
	return 0;
}


