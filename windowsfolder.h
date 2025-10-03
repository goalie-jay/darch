#ifndef WINDOWSFOLDER_H
#define WINDOWSFOLDER_H

#include <Windows.h>

typedef enum
{
	AOT_FILE,
	AOT_DIRECTORY,
	AOT_OTHER
} ABSTRACTOBJECTTYPE;

static char** GetDirectoryObjectsOfType(char* szPath, size_t* nCount, ABSTRACTOBJECTTYPE nObjType)
{
	*nCount = 0;

    size_t nAllocatedSize = 16;
    char** lpArr = malloc(sizeof(char*) * nAllocatedSize);

    if (!lpArr) return NULL;

    // Construct the search path with wildcard
    char szSearchPath[MAX_PATH];
    snprintf(szSearchPath, MAX_PATH, "%s\\*", szPath);

    // Start search
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = NULL;
    hFind = FindFirstFile(szSearchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
    	free(lpArr);
    	return NULL;
    }

    do 
    {
        const char* sz = findFileData.cFileName;

        // Skip "." and ".." entries
        if (strcmp(sz, ".") == 0 || strcmp(sz, "..") == 0)
            continue;

        // This is a long and complicated conditional but basically it checks if we should be including this object
        if ((nObjType == AOT_FILE) ? (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) : (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
        	char szFullPath[MAX_PATH];
			snprintf(szFullPath, MAX_PATH, "%s/%s", szPath, sz);

        	size_t nDirNameLen = strlen(szFullPath);
        	lpArr[*nCount] = malloc(nDirNameLen + 1);
        	if (!lpArr[*nCount])
        	{
        		for (int64_t n = 0l; n < *nCount; ++n)
        			free(lpArr[n]);

        		free(lpArr);
        		return NULL;
        	}

        	memset(lpArr[*nCount], 0, nDirNameLen + 1);
        	memcpy(lpArr[*nCount], szFullPath, nDirNameLen);

        	*nCount += 1;

        	// Check this before the loop repeats so we don't overflow
        	if (*nCount >= nAllocatedSize)
        	{
        		nAllocatedSize *= 2;
        		char** lpNew = realloc(lpArr, sizeof(char*) * nAllocatedSize);
        		if (!lpNew)
        		{
        			for (size_t n = 0l; n < *nCount; n++)
        				free(lpArr[n]);

        			free(lpArr);
        			return NULL;
        		}

        		lpArr = lpNew;
        	}
        }

    } while (FindNextFile(hFind, &findFileData) != 0);

    return lpArr;
}

#endif