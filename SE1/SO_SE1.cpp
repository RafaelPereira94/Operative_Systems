// SO_SE1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Psapi.h>

void PrintMemInfo(int processId);

/* int main(int argc, char * argv[])
{
	if (argc < 1){
		PrintMemInfo(5436); //6672 cmd.exe
	}
	PrintMemInfo(5436);
	return 0;
} */

void PrintMemInfo(int processId){

	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status); //feito antes da chamada a função (dito msdn)
	PERFORMACE_INFORMATION pi;
	if (GlobalMemoryStatusEx(&status)){

		DWORDLONG  totalphys = status.ullTotalPhys;
		printf("Total da memoria fisica existente = %llu B \n", totalphys);

		DWORDLONG  occuphys = totalphys - status.ullAvailPhys;
		printf("Total da memoria fisica ocupada = %llu B \n", occuphys);

		DWORDLONG  totalVirt = status.ullTotalVirtual;
		printf("Total de memoria virtual existente = %llu B \n", totalVirt);

		DWORDLONG  occuVirt = totalVirt - status.ullAvailVirtual;
		printf("Total de memoria virtual ocupada = %llu B \n", occuVirt);

		
		pi.cb = sizeof(PERFORMACE_INFORMATION);
		GetPerformanceInfo(&pi, pi.cb);
		printf("Dimensao das paginas de memoria = %d KB \n", pi.PageSize);

	}
	else return;


	printf("Local information to process %d \n", processId);
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);

	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);

	GetProcessMemoryInfo(handle, &pmc, pmc.cb);

	printf("Total de espaco de enderecamento existente = %d \n");
	
	printf("Total de espaco de enderecamento ocupado = %d \n");

	printf("Numero de (hard) page faults = %ld \n",pmc.PageFaultCount);

	printf("Dimensao total do working set = %d \n",pmc.WorkingSetSize);

	PSAPI_WORKING_SET_INFORMATION psp;
	PSAPI_WORKING_SET_INFORMATION *pv;

	QueryWorkingSet(handle, (PVOID)&psp, sizeof(psp));

	DWORD alloc = sizeof(PSAPI_WORKING_SET_INFORMATION) + sizeof(PSAPI_WORKING_SET_BLOCK) * psp.NumberOfEntries;

	pv = (PSAPI_WORKING_SET_INFORMATION*) malloc(alloc);
	
	int counter = 0; //contador de working set privados
	int privset = 0;
	
	if (QueryWorkingSet(handle, pv, alloc)){
		for (long i = 0; i < pv->NumberOfEntries; ++i){
			if (!pv->WorkingSetInfo[i].Shared){
				++counter;
			}
		}
		privset = counter*pi.PageSize;

	}

	printf("Dimensao do working set privado = %lu \n",privset); // !? precorrer todos os working set blocks !?

	printf("Closing Handle...\n");
	CloseHandle(handle);
}