#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <assert.h>
#include "SearchService.h"
#include "SearchServerApp.h"

static PSearchService service;

UINT _stdcall partialProcessEntry(LPVOID args)
{
	CHAR c;
	DWORD res, bytesReaded;
	PParallelCtx ctx = (PParallelCtx)args;

	PCHAR windowBuffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, ctx->tokenSize + 1);

	while(true)
	{
		int i = InterlockedIncrement(&ctx->currPos);
		if (i >= ctx->size) break;
		PWIN32_FIND_DATA data = ctx->vals + i;

		_tprintf(_T("Client id = %d Search on file: %s\n"), ctx->id, data->cFileName);
		HANDLE hFile = CreateFile(data->cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		assert(hFile != INVALID_HANDLE_VALUE);

		// clear windowBuffer
		memset(windowBuffer, 0, ctx->tokenSize + 1);

		res = ReadFile(hFile, &c, 1, &bytesReaded, NULL);
		while (res && bytesReaded == 1) {

			// slide window to accommodate new char
			memmove_s(windowBuffer, ctx->tokenSize, windowBuffer + 1, ctx->tokenSize - 1);
			windowBuffer[ctx->tokenSize - 1] = c;

			// test accumulated bytes with token
			if (memcmp(windowBuffer, ctx->ent->value, ctx->tokenSize) == 0) {

				// append filename to answer and go to next file

				strcat_s(&*ctx->answer, MAX_CHARS, data->cFileName);
				strcat_s(&*ctx->answer, MAX_CHARS, "\n");
				break;
			}
			res = ReadFile(hFile, &c, 1, &bytesReaded, NULL);
		}
		CloseHandle(hFile);
		
	}
	HeapFree(GetProcessHeap(), 0, windowBuffer);
	return 0;
	
}

/*-----------------------------------------------------------------------
This function allows the processing of a selected set of files in a directory
It uses the Windows functions for directory file iteration, namely
"FindFirstFile" and "FindNextFile"
*/
UINT _stdcall processEntry(LPVOID args) {
	//VOID _stdcall processEntry(PCHAR path, PEntry entry)
	
	PPROCESSENTRY cts = (PPROCESSENTRY)args; //argumentos passados a cada thread.
	
	DWORD numCpus = cts->cpus;

	//PEntry ent = (PEntry)malloc(sizeof(Entry));
	//memcpy(ent, &*cts->entry, sizeof(Entry)); //senao threads trabalham com o mesmo entry

	HANDLE iterator;
	WIN32_FIND_DATA fileData; //Contains information about the file that is found
	TCHAR buffer[MAX_PATH];		// auxiliary buffer
	DWORD tokenSize;

	_tprintf(_T("Token to search: %s\n"), cts->entry->value);

	// the buffer is needed to define a match string that guarantees 
	// a priori selection for all files
	_stprintf_s(buffer, _T("%s\\%s"), cts->path, _T("*.*"));

	// start iteration
	if ((iterator = FindFirstFile(buffer, &fileData)) == INVALID_HANDLE_VALUE)
		goto error;

	// alloc buffer to hold bytes readed from file stream
	tokenSize = strlen(cts->entry->value);
	PCHAR windowBuffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, tokenSize + 1);
	// set auxiliary vars
	PCHAR answer;
	PSharedBlock pSharedBlock = (PSharedBlock)service->sharedMem;
	answer = pSharedBlock->answers[cts->entry->answIdx];
	memset(answer, 0, MAX_CHARS);
	
	PWIN32_FIND_DATA vals = (PWIN32_FIND_DATA) malloc(sizeof(WIN32_FIND_DATA)*30);

	int idx = 0;
	// process only file entries
	do {
		if (fileData.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) {
			vals[idx++] = fileData;
		}
	} while (FindNextFile(iterator, &fileData));

	PParallelCtx ct = (PParallelCtx)malloc(sizeof(ParallelCtx));
	
	ct->id = cts->id;
	ct->currPos = -1;
	ct->vals = vals;
	//ct->windowBuffer = windowBuffer;
	ct->tokenSize = tokenSize;
	ct->answer = answer;
	ct->ent = cts->entry;
	ct->size = idx;

	HANDLE * threads = (HANDLE*)malloc(sizeof(HANDLE) * numCpus);
	for (DWORD i = 0; i < numCpus; i++) {
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, partialProcessEntry, ct, 0, NULL);
	}

	WaitForMultipleObjects(numCpus, threads, TRUE, INFINITE);

	// sinalize client and finish answer
	SetEvent(cts->entry->answReadyEvt);
	CloseHandle(cts->entry->answReadyEvt);

	FindClose(iterator);
	//HeapFree(GetProcessHeap(), 0, windowBuffer);
	
	free(cts->entry);
	free(cts);
	for (DWORD i = 0; i < numCpus; i++) {
		CloseHandle(threads[i]);
	}
	free(threads);

error:
	//free(ent);
	return 0;

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
	DWORD id = 0;
	
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD numCpus = si.dwNumberOfProcessors; //numero de cpus


	Entry entry;
	while (1) {
		res = SearchGet(service, &entry);
		if (res == FALSE)
			break;

		PPROCESSENTRY cts = (PROCESSENTRY*) malloc(sizeof(PROCESSENTRY));
		cts->path = path;

		PEntry ent = (PEntry) malloc(sizeof(Entry));
		memcpy(ent, &entry, sizeof(Entry)); //senao threads trabalham com o mesmo entry

		cts->entry = ent;
		cts->id = id++;
		cts->cpus = numCpus;

		_beginthreadex(NULL,0,processEntry,(LPVOID)cts, 0, NULL); 

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
