#include "stdafx.h"
#include "DllPrint.h"
#include <Windows.h>

void printDll();

int main(int argc, char * argv[]){

	printDll();
	return 0;

}

void printDll(){
	
	PIMAGE_DOS_HEADER idm  = (PIMAGE_DOS_HEADER) GetModuleHandle(NULL);

	PIMAGE_NT_HEADERS inh = (PIMAGE_NT_HEADERS) ((BYTE*)idm + idm->e_lfanew);

	PIMAGE_OPTIONAL_HEADER iph = (PIMAGE_OPTIONAL_HEADER) &inh->OptionalHeader;

	PIMAGE_DATA_DIRECTORY pidd = (PIMAGE_DATA_DIRECTORY) &iph->DataDirectory[1];

	PIMAGE_IMPORT_DESCRIPTOR iid = (PIMAGE_IMPORT_DESCRIPTOR) ((BYTE*)idm + pidd->VirtualAddress);

	printf("%s",(CHAR*)((BYTE*)idm+iid->Name)); //name da dll.

	
}

