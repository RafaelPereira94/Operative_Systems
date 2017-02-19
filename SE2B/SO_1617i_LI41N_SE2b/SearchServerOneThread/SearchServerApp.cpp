#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <assert.h>
#include "SearchService.h"
#include "SearchServerApp.h"

static PSearchService service;


/*-----------------------------------------------------------------------
This function allows the processing of a selected set of files in a directory
It uses the Windows functions for directory file iteration, namely
"FindFirstFile" and "FindNextFile"
*/
UINT _stdcall processEntry(LPVOID args) {
	//VOID _stdcall processEntry(PCHAR path, PEntry entry)
	
	DWORD numCpus = 0;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	numCpus = si.dwNumberOfProcessors; //numero de cpus

	PROCESSENTRY *cts = (PPROCESSENTRY)args; //argumentos passados a cada thread.
	PCHAR path = cts->path;
	PEntry entry = (PEntry) malloc(sizeof(Entry));
	memcpy(entry, cts->entry, sizeof(Entry)); //senao threads trabalham com o mesmo entry

	HANDLE iterator;
	WIN32_FIND_DATA fileData;
	TCHAR buffer[MAX_PATH];		// auxiliary buffer
	DWORD tokenSize;

	_tprintf(_T("Token to search: %s\n"), entry->value);

	// the buffer is needed to define a match string that guarantees 
	// a priori selection for all files
	_stprintf_s(buffer, _T("%s\\%s"), path, _T("*.*"));

	// start iteration
	if ((iterator = FindFirstFile(buffer, &fileData)) == INVALID_HANDLE_VALUE)
		goto error;

	// alloc buffer to hold bytes readed from file stream
	tokenSize = strlen(entry->value);
	PCHAR windowBuffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, tokenSize + 1);
	// set auxiliary vars
	PCHAR answer;
	PSharedBlock pSharedBlock = (PSharedBlock)service->sharedMem;
	answer = pSharedBlock->answers[entry->answIdx];
	memset(answer, 0, MAX_CHARS);

	// process only file entries
	do {
		if (fileData.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) {
			CHAR c;
			DWORD res, bytesReaded;

			_tprintf(_T("Search on file: %s\n"), fileData.cFileName);
			HANDLE hFile = CreateFile(fileData.cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			assert(hFile != INVALID_HANDLE_VALUE);

			// clear windowBuffer
			memset(windowBuffer, 0, tokenSize + 1);

			res = ReadFile(hFile, &c, 1, &bytesReaded, NULL);
			while (res && bytesReaded == 1) {
				
				// slide window to accommodate new char
				memmove_s(windowBuffer, tokenSize, windowBuffer + 1, tokenSize - 1);
				windowBuffer[tokenSize - 1] = c;

				// test accumulated bytes with token
				if (memcmp(windowBuffer, entry->value, tokenSize) == 0) {
					
					// append filename to answer and go to next file
					strcat_s(answer, MAX_CHARS, fileData.cFileName);
					strcat_s(answer, MAX_CHARS, "\n");
					break;
				}
				res = ReadFile(hFile, &c, 1, &bytesReaded, NULL);
			}
			CloseHandle(hFile);
		}
	} while (FindNextFile(iterator, &fileData));

	// sinalize client and finish answer
	SetEvent(entry->answReadyEvt);
	CloseHandle(entry->answReadyEvt);

	FindClose(iterator);
	HeapFree(GetProcessHeap(), 0, windowBuffer);
	
	free(entry);

	return 0;

error:
	;
	return 1;
}

INT main(DWORD argc, PCHAR argv[]) {
	PCHAR name;
	PCHAR path;
	DWORD res;
	CHAR pathname[MAX_CHARS*4];

	if (argc < 3) {
		printf("Use > %s <service_name> <repository pathname>\n", argv[0]);
		name = "Service1";
		res = GetCurrentDirectory(MAX_CHARS, pathname);
		assert(res > 0);
		path = pathname;
		printf("Using > %s %s \"%s\"\n", argv[0], name, path);
	}
	else {
		name = argv[1];
		path = argv[2];
	}
	printf("Server app: Create service with name = %s. Repository name = %s\n", name, path);
	service = SearchCreate(name); assert(service != NULL);

	Entry entry;
	while (1) {
		res = SearchGet(service, &entry);
		if (res == FALSE)
			break;
		PPROCESSENTRY cts = (PROCESSENTRY*) malloc(sizeof(PROCESSENTRY));
		cts->path = path;
		cts->entry = &entry;

		_beginthreadex(NULL,0,processEntry,cts, 0, NULL); 

		//processEntry(path, &entry);
	}
	//for (int i = 0; i < MAX_SERVERS; i++) {
	//	threads[i] = (HANDLE)_beginthreadex(NULL, 0, server_thread, (LPVOID)i, 0, NULL);
	//}

	//res = WaitForMultipleObjects(MAX_SERVERS, threads, TRUE, INFINITE);
	printf("Server app: Close service name = %s and exit\n", name);
	SearchClose(service);

	return 0;
}
