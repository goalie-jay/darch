#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <stdint.h>
#include <stdio.h>

#define ARCHIVE_INT int64_t

typedef struct
{
	ARCHIVE_INT Magic;
	ARCHIVE_INT RelativePathLength;
	char* RelativePathASCII;
	ARCHIVE_INT Permissions;
	ARCHIVE_INT BinaryLength;
	char* BinaryContents;
} ARCHIVE_FILE_ENTRY;

typedef struct
{
	ARCHIVE_INT Magic;
	ARCHIVE_INT EntryCount;
	ARCHIVE_FILE_ENTRY** Entries;
	ARCHIVE_INT __EntriesAllocated;
} ARCHIVE_HEADER;

void ARCHIVER_SetVerbose(char);
void ARCHIVER_Archive(char** objectNames, long objectCount, FILE* output);
void ARCHIVER_Extract(FILE* input, char* outputDir);

#endif