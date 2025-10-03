#include "archiver.h"

#include <assert.h>

#ifdef WINDOWS
#include "windowsfolder.h"
#else
#include "posixfolder.h"
#include <sys/errno.h>
#include <sys/stat.h>
#endif

#include <string.h>
#include "error.h"

#ifdef WINDOWS
const char PLATFORM_SLASH = '\\';
#else
const char PLATFORM_SLASH = '/';
#endif
const ARCHIVE_INT HEADER_MAGIC = 0x992FF;
const int INITIAL_ENTRY_ALLOC_COUNT = 20;
char Verbose = 0;

void ARCHIVE_SetVerbose(char verbose)
{
	Verbose = verbose;
}

char IsDirectory(char* sz)
{
#ifdef WINDOWS
	DWORD dw = GetFileAttributesA(sz);

	if (dw == INVALID_FILE_ATTRIBUTES) return 0;
	if (dw & FILE_ATTRIBUTE_DIRECTORY) return 1;

	return 0;
#else
	DIR* lp = opendir(sz);
	if (!lp) return 0;

	closedir(lp);
	return 1;
#endif
}

char IsFile(char* sz)
{
#ifdef WINDOWS
	DWORD dw = GetFileAttributesA(sz);

	if (dw == INVALID_FILE_ATTRIBUTES) return 0;
	if (dw & FILE_ATTRIBUTE_DIRECTORY) return 0;

	return 1;
#else
	return (access(sz, 0) == 0) && !IsDirectory(sz);
#endif
}

// Helper method
ARCHIVE_INT readint(FILE* stream)
{
	static char buf[sizeof(ARCHIVE_INT)];
	fread(buf, sizeof(ARCHIVE_INT), 1, stream);

	ARCHIVE_INT n;
	memcpy(&n, buf, sizeof(ARCHIVE_INT));
	return n;
}

char* readarr(FILE* stream, ARCHIVE_INT n, ARCHIVE_INT sled)
{
	char* lp = malloc(n + sled);
	assert(lp);

	memset(lp, 0, n + sled);
	fread(lp, 1, n, stream);

	return lp;
}

void writeint(FILE* stream, ARCHIVE_INT n)
{
	fwrite(&n, 1, sizeof(ARCHIVE_INT), stream);
}

void writearr(FILE* stream, char* lp, ARCHIVE_INT n)
{
	fwrite(lp, 1, n, stream);
}

ARCHIVE_FILE_ENTRY* FILEENTRY_Create()
{
	return malloc(sizeof(ARCHIVE_FILE_ENTRY));
}

void FILEENTRY_Destroy(ARCHIVE_FILE_ENTRY* lp)
{
	free(lp->RelativePathASCII);
	free(lp->BinaryContents);
	free(lp);
}

void HEADER_AddEntry(ARCHIVE_HEADER* lp, 
	ARCHIVE_FILE_ENTRY* entry)
{
	if (!lp->Entries)
	{
		lp->Entries = malloc(sizeof(ARCHIVE_FILE_ENTRY*) *INITIAL_ENTRY_ALLOC_COUNT);
		lp->__EntriesAllocated = INITIAL_ENTRY_ALLOC_COUNT;
	}
	else if (lp->EntryCount + 1 >= lp->__EntriesAllocated)
	{
		lp->__EntriesAllocated *= 2;
		lp->Entries = realloc(lp->Entries, sizeof(ARCHIVE_FILE_ENTRY*) * lp->__EntriesAllocated);
	}

	lp->Entries[lp->EntryCount] = entry;
	++lp->EntryCount;
}

void HEADER_AddFile(ARCHIVE_HEADER* lp, char* relativePath, char* binaryContents, ARCHIVE_INT size, ARCHIVE_INT permissions)
{
	ARCHIVE_FILE_ENTRY* entry = FILEENTRY_Create();

	entry->Magic = HEADER_MAGIC;
	entry->RelativePathLength = strlen(relativePath);
	entry->RelativePathASCII = relativePath;
	entry->Permissions = permissions;
	entry->BinaryLength = size;
	entry->BinaryContents = binaryContents;

	HEADER_AddEntry(lp, entry);
}

ARCHIVE_HEADER* HEADER_Create()
{
	ARCHIVE_HEADER* lp = malloc(sizeof(ARCHIVE_HEADER));
	memset(lp, 0, sizeof(ARCHIVE_HEADER));

	lp->Magic = HEADER_MAGIC;
	lp->EntryCount = 0;
	lp->Entries = NULL;
	lp->__EntriesAllocated = -1;

	return lp;
}

void HEADER_Destroy(ARCHIVE_HEADER* lp)
{
	if (lp->Entries) for (int i = 0; i < lp->EntryCount; ++i) FILEENTRY_Destroy(lp->Entries[i]);
	free(lp);
}

void ReconstructArchiveDataFromFile(ARCHIVE_HEADER* lp, FILE* input)
{
	ARCHIVE_INT headerMagic = readint(input);
	if (headerMagic != HEADER_MAGIC)
	{
		puts("Magic value did not match.");
		exit(DARCH_ERROR_BAD_MAGIC_VALUE);
	}

	ARCHIVE_INT fileCount = readint(input);

	for (ARCHIVE_INT i = 0; i < fileCount; ++i)
	{
		ARCHIVE_FILE_ENTRY* entry = FILEENTRY_Create();

		entry->Magic = readint(input);
		if (entry->Magic != HEADER_MAGIC)
		{
			puts("Magic value did not match.");
			exit(DARCH_ERROR_BAD_MAGIC_VALUE);
		}

		entry->RelativePathLength = readint(input);
		entry->RelativePathASCII = readarr(input, entry->RelativePathLength, 1);
		entry->Permissions = readint(input);
		entry->BinaryLength = readint(input);
		entry->BinaryContents = readarr(input, entry->BinaryLength, 1);

		HEADER_AddEntry(lp, entry);
	}
}

ARCHIVE_INT PathGetPLATFORM_SLASHIdx(char* sz)
{
	ARCHIVE_INT i = strlen(sz) - 1;
	while (i >= 0 && sz[i] != PLATFORM_SLASH)
		--i;

	return i;
}

char* GetFileNameFromPath(char* sz)
{
	ARCHIVE_INT PLATFORM_SLASHIdx = PathGetPLATFORM_SLASHIdx(sz);

	if (sz[PLATFORM_SLASHIdx + 1] == 0) return NULL;

	ARCHIVE_INT allocSize = strlen(sz) - PLATFORM_SLASHIdx;
	char* lp = malloc(allocSize);
	memset(lp, 0, allocSize);

	memcpy(lp, (char*)((uint64_t)sz + PLATFORM_SLASHIdx + 1), allocSize - 1);
	return lp;
}

char* GetDirectoryNameFromPath(char* sz)
{
	ARCHIVE_INT PLATFORM_SLASHIdx = PathGetPLATFORM_SLASHIdx(sz);

	char* lp = malloc(PLATFORM_SLASHIdx + 1);
	memset(lp, 0, PLATFORM_SLASHIdx + 1);
	memcpy(lp, sz, PLATFORM_SLASHIdx);

	return lp;
}

char* CombinePath(char* left, char* right)
{
	size_t lenLeft = strlen(left), lenRight = strlen(right);
	char* lp = malloc(lenLeft + lenRight + 2);
	memset(lp, 0, lenLeft + lenRight + 2);
	char* lpSecondStr = NULL;

	memcpy(lp, left, lenLeft);
	if (lp[lenLeft - 1] == PLATFORM_SLASH)
		lpSecondStr = (char*)((uint64_t)lp + lenLeft);
	else
	{
		lp[lenLeft] = PLATFORM_SLASH;
		lpSecondStr = (char*)((uint64_t)lp + lenLeft + 1);
	}

	memcpy(lpSecondStr, right, lenRight);

	return lp;
}

void AddFileToArchive(ARCHIVE_HEADER* lp, char* filename, char* parent)
{
#ifdef WINDOWS
	if (Verbose)
		printf("%s\n", filename);

	ARCHIVE_INT permissions = 0; // Skip on Windows because it's unnecessary
	FILE* f = fopen(filename, "rb");
	ARCHIVE_INT size = 0;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* binary = malloc(size);
	fread(binary, 1, size, f);

	char* fname = GetFileNameFromPath(filename);
	char* relativePath = CombinePath(parent, fname);
	free(fname);
	fclose(f);
	HEADER_AddFile(lp, relativePath, binary, size, permissions);
#else
	struct stat s;
	if (stat(filename, &s) != -1)
	{
		if (Verbose)
			printf("%s\n", filename);

		ARCHIVE_INT permissions = s.st_mode;
		FILE* f = fopen(filename, "rb");
		ARCHIVE_INT size = 0;
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* binary = malloc(size);
		fread(binary, 1, size, f);

		char* fname = GetFileNameFromPath(filename);
		char* relativePath = CombinePath(parent, fname);
		free(fname);
		fclose(f);
		HEADER_AddFile(lp, relativePath, binary, size, permissions);
	}
#endif
}

void AddDirectoryRecursiveToArchive(ARCHIVE_HEADER* lp, char* dir, char* parent)
{
	size_t fileCount, dirCount;
	char** files = GetDirectoryObjectsOfType(dir, &fileCount, AOT_FILE);
	char** dirs = GetDirectoryObjectsOfType(dir, &dirCount, AOT_DIRECTORY);

	char* justDirName = GetFileNameFromPath(dir);
	char* nextParent = CombinePath(parent, justDirName);

	for (int i = 0; i < fileCount; ++i) AddFileToArchive(lp, files[i], nextParent);
	for (int i = 0; i < dirCount; ++i) AddDirectoryRecursiveToArchive(lp, dirs[i], nextParent);
	free(nextParent);
	free(justDirName);
}

void WriteArchiveToFile(ARCHIVE_HEADER* lp, FILE* stream)
{
	writeint(stream, lp->Magic);
	writeint(stream, lp->EntryCount);

	for (int i = 0; i < lp->EntryCount; ++i)
	{
		ARCHIVE_FILE_ENTRY* lpEntry = lp->Entries[i];
		writeint(stream, lpEntry->Magic);
		writeint(stream, lpEntry->RelativePathLength);
		writearr(stream, lpEntry->RelativePathASCII, lpEntry->RelativePathLength);
		writeint(stream, lpEntry->Permissions);
		writeint(stream, lpEntry->BinaryLength);
		writearr(stream, lpEntry->BinaryContents, lpEntry->BinaryLength);
	}
}

void ARCHIVER_Archive(char** objectNames, long objectCount, FILE* output)
{
	ARCHIVE_HEADER* lp = HEADER_Create();

	for (int i = 0; i < objectCount; ++i)
	{
		if (IsFile(objectNames[i])) AddFileToArchive(lp, objectNames[i], ".");
		else if (IsDirectory(objectNames[i])) AddDirectoryRecursiveToArchive(lp, objectNames[i], ".");
		else
		{
			printf("Object %s queued for archiving could not be found.\n", objectNames[i]);
			exit(DARCH_ERROR_INACCESSIBLE_FS_OBJECT);
		}
	}

	WriteArchiveToFile(lp, output);
	HEADER_Destroy(lp);
}

#ifdef WINDOWS
char mkdir_p(char* path, long mode_WARNING_UNUSED_ON_THIS_PLATFORM)
#else
char mkdir_p(char* path, mode_t mode)
#endif
{
	char *tmp = strdup(path);
	char *p = NULL;
	size_t len;

	if (tmp == NULL)
		return 0;

	len = strlen(tmp);
	if (len == 0)
	{
		free(tmp);
		return 0;
	}

	// Remove trailing slash
	if (tmp[len - 1] == PLATFORM_SLASH)
		tmp[len - 1] = '\0';

	for (p = tmp + 1; *p; p++)
	{
		if (*p == PLATFORM_SLASH)
		{
			*p = '\0';

#ifdef WINDOWS
			if (!CreateDirectoryA(tmp, NULL))
			{
				if (GetLastError() != DARCH_ERROR_ALREADY_EXISTS)
				{
					free(tmp);
					return 0;
				}
			}
#else
			if (mkdir(tmp, mode) != 0)
			{
				if (errno != EEXIST)
				{
					free(tmp);
					return 0;
				}
			}
#endif

			*p = PLATFORM_SLASH;
		}
	}

	// Make final directory
#ifdef WINDOWS
	if (!CreateDirectoryA(tmp, NULL))
	{
		if (GetLastError() != DARCH_ERROR_ALREADY_EXISTS)
		{
			free(tmp);
			return 0;
		}
	}
#else
	if (mkdir(tmp, mode) != 0)
	{
		if (errno != EEXIST)
		{
			free(tmp);
			return 0;
		}
	}
#endif

	free(tmp);
	return 1;
}

void ARCHIVER_Extract(FILE* input, char* outputDir)
{
	ARCHIVE_HEADER* lp = HEADER_Create();
	ReconstructArchiveDataFromFile(lp, input);

	if (!IsDirectory(outputDir) && !mkdir_p(outputDir, 0777))
	{
		puts("Could not create output directory.");
		exit(DARCH_ERROR_OUTPUT_DIRECTORY_CREATION_FAILURE);
	}

	for (int i = 0; i < lp->EntryCount; ++i)
	{
		char* destination = CombinePath(outputDir, lp->Entries[i]->RelativePathASCII);
		char* destinationParent = GetDirectoryNameFromPath(destination);

		if (!IsDirectory(destinationParent) && !mkdir_p(destinationParent, 0777))
		{
			printf("Failed to create directory %s.\n", destinationParent);
			exit(DARCH_ERROR_OUTPUT_DIRECTORY_CREATION_FAILURE);
		}

		FILE* f = fopen(destination, "wb");
		if (!f)
		{
			printf("Failed to write file %s.\n", destination);
			exit(DARCH_ERROR_FILE_ACCESS);
		}

		fwrite(lp->Entries[i]->BinaryContents, 1, lp->Entries[i]->BinaryLength, f);
		fclose(f);

#ifndef WINDOWS
		chmod(destination, (mode_t)lp->Entries[i]->Permissions);
#endif

		free(destination);
		free(destinationParent);
	}

	HEADER_Destroy(lp);
}