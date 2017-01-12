#include <Windows.h>
#include <stdio.h>
#include <process.h> 
#include "WindowsThreads.h"

DWORD main(DWORD argc, PCHAR argv[]) {
	HANDLE hThread, hThread1,hThread2;
	unsigned threadID;
	PDWORD count = 0;
	hThread = (HANDLE) _beginthreadex(NULL, 0, Thread_func, &count, 0, &threadID);
	hThread1 = (HANDLE)_beginthreadex(NULL, 0, Thread_func1, NULL, 0, &threadID);
	hThread2 = (HANDLE)_beginthreadex(NULL, 0, Thread_func2, NULL, 0, &threadID);

	CloseHandle(hThread);
	CloseHandle(hThread1);
	CloseHandle(hThread2);
	getchar();

	return 0;
}

unsigned __stdcall Thread_func(void *ArgList) {
	int time = GetTickCount();
	int count = *((PDWORD)ArgList);
	for (int i = 0;i <1234567;++i)
		count++;

	printf("Thread1 executed in %d milliseconds\n", (GetTickCount() - time));

	_endthreadex(0);
	return 0;
}

unsigned Counter;
unsigned __stdcall Thread_func1(void *ArgList) {
	int time = GetTickCount();
	
	Sleep(5000);

	printf("Thread2 executed in %d milliseconds\n", (GetTickCount() - time));

	_endthreadex(0);
	return 0;
}


unsigned __stdcall Thread_func2(void *ArgList) {
	int time = GetTickCount();
	long count = 0;
	for (long i = 0 ; i<LONG_MAX;++i)
		++count;

	printf("Thread3 executed in %d milliseconds\n", (GetTickCount() - time));

	_endthreadex(0);
	return 0;
}