#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <assert.h>
#include "SearchService.h"
#include "CompletionPort.h"

// this is a "black magic" alternative to explicit link with sockets library
#pragma comment (lib, "Ws2_32.lib")

// Maximum operations in cocurrency
#define MAX_OPERATIONS	10
// MAX_WINDOW is the number of bytes to read in each ReadFile access. 
// The size of this buffer must be greater than the value to search fixed to MAX_CHARS (see SearchService.h)
#define MAX_WINDOW		MAX_CHARS * 10
static PSearchService service;

typedef struct search_session {
	PEntry req;
	PCHAR answer;					// Hold final answer to be returned
	DWORD answerLen;
	ULONG operationsInProgress;		// Hold number of concurrent operations
	HANDLE repositoryIt;			// Hold iterator to FindNextFile
	CRITICAL_SECTION cs;
} SEARCH_SESSION, * PSEARCH_SESSION;

typedef struct aio_oper {
	CHAR   partialAnswer[MAX_CHARS];// Hold filenames collected in this operation
	DWORD  partialAnswerLen;
	CHAR   filename[MAX_PATH];		// Hold temporary filename
	HANDLE file;					// Hold file handle in search process
	CHAR  buffer[MAX_WINDOW];		// Hold bytes read from file
	CHAR  secondbuffer[MAX_WINDOW];
	OVERLAPPED ov;
	PSEARCH_SESSION searchSession;
} SEARCH_OPER, * PSEARCH_OPER;

VOID cb(struct _aio_dev* aiodev, INT transferedBytes, LPVOID ctx)
{
	PSEARCH_OPER op = (PSEARCH_OPER)ctx;

	if (transferedBytes <= 0)
		SearchSessionProcessNextFile(&op->ov);
	else
		//SearchSessionProcessCurrentFile(ovr, bytesRead);
		SearchSessionProcessCurrentFile(&op->ov, transferedBytes);
}

PSEARCH_OPER SearchSessionOperationNew(HANDLE file, PCHAR filename, PSEARCH_SESSION session) {
	PSEARCH_OPER oper = (PSEARCH_OPER)malloc(sizeof(SEARCH_OPER));
	oper->partialAnswerLen = 0;
	oper->partialAnswer[0] = 0;
	oper->searchSession = session;
	ZeroMemory(&oper->ov, sizeof(OVERLAPPED));
	ZeroMemory(oper->buffer, sizeof(oper->buffer));
	strcpy_s(oper->filename, sizeof(oper->filename), filename);
	session->operationsInProgress += 1; // Increment number of operations in progess
	//CompletionPortAssociateHandle(file, session); // the second argument IS NOT an issue
	PAIO_DEV paio = (PAIO_DEV)malloc(sizeof(AIO_DEV));
	InitAioDev(paio, file, false);
	SetAioOper(paio, cb, oper); //associa cb

	oper->file = file;

	return oper;
}
PSEARCH_SESSION SearchSessionNew(PEntry entry, HANDLE repositoryIterator, PCHAR answer) {
	PSEARCH_SESSION session = (PSEARCH_SESSION)malloc(sizeof(SEARCH_SESSION));
	session->req = entry;
	InitializeCriticalSection(&session->cs);
	session->repositoryIt = repositoryIterator;
	session->operationsInProgress = 0;
	session->answer = answer;
	session->answerLen = 0;

	return session;
}

VOID SearchSessionOperationTerminate(PSEARCH_OPER oper) {
	PSEARCH_SESSION session = oper->searchSession;
	PCHAR partialAnsw = oper->partialAnswer;
	DWORD partialAnswLen = oper->partialAnswerLen;
	// copy partial answer to common answer 
	EnterCriticalSection(&session->cs);
	PCHAR answer = session->answer;
	DWORD answerLen = session->answerLen;
	strcpy_s(answer + answerLen, MAX_CHARS - answerLen, partialAnsw);
	session->answerLen += partialAnswLen;
	LeaveCriticalSection(&session->cs);
	free(oper);
}
VOID SearchSessionTerminate(PSEARCH_SESSION session) {
	DeleteCriticalSection(&session->cs);
	FindClose(session->repositoryIt);
	PEntry entry = session->req;
	// sinalize client and finish answer
	SetEvent(entry->answReadyEvt);
	CloseHandle(entry->answReadyEvt);
	free(entry);
	free(session);
}


// Returns first file in repository if exists. Return false if there are no files in repository.
BOOL GetFirstFile(PCHAR path, _Out_ HANDLE * repositoryIterator, _Out_ HANDLE * file, _Out_ PWIN32_FIND_DATA fileData) {
	HANDLE iterator;

	// the buffer is needed to define a match string that guarantees 
	// a priori selection for all files
	TCHAR buffer[MAX_PATH];		// auxiliary buffer
	_stprintf_s(buffer, _T("%s\\%s"), path, _T("*.*"));

	// start iteration 
	if ((iterator = FindFirstFile(buffer, fileData)) == INVALID_HANDLE_VALUE)
		return false;

	// Iterate until get a file archive
	do {
		if (fileData->dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) {

			_tprintf(_T("Search on file: %s\n"), fileData->cFileName);
			HANDLE hFile = CreateFile(fileData->cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
			assert(hFile != INVALID_HANDLE_VALUE);

			// return repository handle and first file handle
			*repositoryIterator = iterator;
			*file = hFile;
			return true;
		}
	} while (FindNextFile(iterator, fileData)); 

	return false;
}

static BOOL FindNextFileTS(HANDLE iterator, PWIN32_FIND_DATA fileData, PCRITICAL_SECTION lock) {
	BOOL res;
	if (lock != NULL) EnterCriticalSection(lock);
	res = FindNextFile(iterator, fileData);
	if (lock != NULL) LeaveCriticalSection(lock);

	return res;
}
// Returns next file in repository if exists. Return false if there are no more files to iterate.
BOOL GetNextFile(HANDLE repositoryIterator, _Out_ HANDLE * file, _Out_ PWIN32_FIND_DATA fileData, PCRITICAL_SECTION lock) {

	// Iterate until get a file archive
	while (FindNextFileTS(repositoryIterator, fileData, lock)) {
		if (fileData->dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) {

			_tprintf(_T("Search on file: %s\n"), fileData->cFileName);
			HANDLE hFile = CreateFile(fileData->cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
			assert(hFile != INVALID_HANDLE_VALUE);

			// return file handle
			*file = hFile;
			return true;
		}
	} 

	return false;
}

// Called in the context of a search operation.
// Finish search in the current file and transfer search to the next file.
VOID SearchSessionProcessNextFile(LPOVERLAPPED ovr) {
	DWORD dummy;
	PSEARCH_OPER oper = CONTAINING_RECORD(ovr, SEARCH_OPER, ov);
	PSEARCH_SESSION session = oper->searchSession;
	WIN32_FIND_DATA findData;
	HANDLE newFile;
	// close current file
	CloseHandle(oper->file);
	BOOL res = GetNextFile(session->repositoryIt, &newFile, &findData, &session->cs);
	if (res) {
		// process next file 
		// reuse the same operation object
		oper->file = newFile;
		ZeroMemory(ovr, sizeof(OVERLAPPED));
		strcpy_s(oper->filename, sizeof(oper->filename), findData.cFileName);
		// associate next file to iocp
		CompletionPortAssociateHandle(oper->file, session); // the second argument IS NOT an issue for this implementation
		
		PAIO_DEV piov = (PAIO_DEV)malloc(sizeof(_aio_dev));
		InitAioDev(piov, oper, false);
		
		res = ReadAsync(piov, oper->buffer, MAX_WINDOW, cb,oper);

		// place first read
		//res = ReadFile(oper->file, oper->buffer, MAX_WINDOW, &dummy, &oper->ov);
		assert(res == false && GetLastError() == ERROR_IO_PENDING);

		return;
	}

	// no more files to process so terminate current operation
	SearchSessionOperationTerminate(oper);

	// if it is the last operation in progress terminate also the session
	DWORD operationsAlive = InterlockedDecrement(&session->operationsInProgress);
	if (operationsAlive == 0)
		SearchSessionTerminate(session);

}

// Called in the context of a search operation.
// Process current chunck of bytes.
// THIS CODE DON'T DETECT A SEQUENCE BROKEN IN TWO BLOCKS OF BYTES.
VOID SearchSessionProcessCurrentFile(LPOVERLAPPED ovr, DWORD transferredBytes) {
	PSEARCH_OPER oper = CONTAINING_RECORD(ovr, SEARCH_OPER, ov);
	PSEARCH_SESSION session = oper->searchSession;
	PEntry entry = session->req;
	
	CHAR c[MAX_WINDOW*2];
	memcpy(c, oper->buffer, MAX_WINDOW);
	memcpy(&c[MAX_WINDOW], oper->secondbuffer, MAX_WINDOW);

	// search for an occurrence
	//oper->buffer[transferredBytes] = 0;
	PCHAR res = strstr(oper->buffer, entry->value);
	if (res != NULL) {
		// register current filename and go to next file if get an occurrence
		strcpy_s(oper->partialAnswer + oper->partialAnswerLen, sizeof(oper->partialAnswer) - oper->partialAnswerLen, oper->filename);
		oper->partialAnswerLen += strlen(oper->filename);
		oper->partialAnswer[oper->partialAnswerLen++] = '\n';
		oper->partialAnswer[oper->partialAnswerLen] = 0;
		SearchSessionProcessNextFile(ovr);
		return;
	}

	if(oper->secondbuffer[0] != NULL)
	{
		_memccpy(oper->buffer,oper->secondbuffer,NULL,MAX_WINDOW);
	}

	// not found! read next chunck of bytes
	LARGE_INTEGER pos;
	DWORD dummy;
	pos.HighPart = ovr->OffsetHigh; pos.LowPart = ovr->Offset;
	pos.QuadPart += transferredBytes;
	ovr->OffsetHigh = pos.HighPart; ovr->Offset = pos.LowPart;
	// place next read
	BOOL res1 = ReadFile(oper->file, oper->secondbuffer, MAX_WINDOW, &dummy, ovr);
	assert(res1 == false && GetLastError() == ERROR_IO_PENDING);


}


// Create a new search session and start a concurrent search process
VOID processEntry(PCHAR path, PEntry entry) {
	HANDLE iterator, firstFile;
	PSEARCH_OPER concurrentOperations[MAX_OPERATIONS];
	DWORD concurrentOperLen = 0;

	// set auxiliary vars
	PCHAR answer;
	PSharedBlock pSharedBlock = (PSharedBlock)service->sharedMem;
	answer = pSharedBlock->answers[entry->answIdx];
	ZeroMemory(answer, MAX_CHARS);

	_tprintf(_T("Token to search: %s\n"), entry->value);

	WIN32_FIND_DATA findData;
	BOOL res = GetFirstFile(path, &iterator, &firstFile, &findData);
	if (res == false) 
		return;
	// create new search session
	PSEARCH_SESSION session = SearchSessionNew(entry, iterator, answer);

	concurrentOperations[concurrentOperLen++] = SearchSessionOperationNew(firstFile, findData.cFileName, session);
	// create concurrent operations
	HANDLE file;
	for (int i = 1; i < MAX_OPERATIONS && GetNextFile(iterator, &file, &findData, NULL) == TRUE; i++) 
		concurrentOperations[concurrentOperLen++] = SearchSessionOperationNew(file, findData.cFileName, session);

	// start operations only after all operations are created. Why should this be an issue?
	for (DWORD i = 0; i < concurrentOperLen; i++) {
		DWORD dummy;
		PSEARCH_OPER oper = concurrentOperations[i];
		res = ReadFile(oper->file, oper->buffer, MAX_WINDOW, &dummy, &oper->ov);
		assert(res == false && GetLastError() == ERROR_IO_PENDING);
	}

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

	// create threadpool with iocp
	CompletionPortCreate(MAX_CONCURRENCY);

	while (1) {
		PEntry entry = (PEntry)malloc(sizeof(Entry));
		res = SearchGet(service, entry);
		if (res == FALSE)
			break;

		processEntry(path, entry);
	}

	printf("Server app: Close service name = %s and exit\n", name);
	SearchClose(service);
	CompletionPortClose();

	return 0;
}
