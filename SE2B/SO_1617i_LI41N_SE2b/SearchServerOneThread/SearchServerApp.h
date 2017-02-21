#pragma once

typedef struct {
	PCHAR path;
	PEntry entry;
	DWORD id; //client id
	DWORD cpus; //number of cpus
}PROCESSENTRY, *PPROCESSENTRY;

typedef struct
{
	PWIN32_FIND_DATA vals; //WIN32_FIND_DATA
	LONG currPos; //posição corrente.
	DWORD id; //id client
	DWORD size;
	PCHAR windowBuffer;
	DWORD tokenSize;
	PCHAR answer;
	PEntry ent;
}ParallelCtx, *PParallelCtx;

UINT _stdcall processEntry(LPVOID args);

UINT _stdcall parallelProcEntry(LPVOID args);



