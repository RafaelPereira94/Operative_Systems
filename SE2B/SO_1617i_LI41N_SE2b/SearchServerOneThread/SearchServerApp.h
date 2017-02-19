#pragma once

typedef struct {
	PCHAR path;
	PEntry entry;
}PROCESSENTRY, *PPROCESSENTRY;

UINT _stdcall processEntry(LPVOID args);

