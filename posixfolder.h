#ifndef POSIXFOLDER_H
#define POSIXFOLDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

typedef enum
{
	AOT_FILE,
	AOT_DIRECTORY,
	AOT_OTHER
} ABSTRACTOBJECTTYPE;

typedef enum
{
	SYMLINKTARGET_FILE,
	SYMLINKTARGET_DIRECTORY,
	SYMLINKTARGET_OTHER,
	SYMLINKTARGET_ERROR
} SYMLINKTARGET;

static SYMLINKTARGET __GetSymbolicLinkTargetType(char* szPath, long nDepth)
{
	if (nDepth > 15)
		return SYMLINKTARGET_ERROR;

    char szTargetPath[PATH_MAX + 1];
	memset(szTargetPath, 0, PATH_MAX + 1);

    ssize_t nLen;

    nLen = readlink(szPath, szTargetPath, PATH_MAX);
    if (nLen == -1)
        return SYMLINKTARGET_ERROR;

    szTargetPath[PATH_MAX] = (char)0; // To be safe

    struct stat sb;

    if (stat(szTargetPath, &sb) == -1)
    	return SYMLINKTARGET_ERROR;

    if (S_ISREG(sb.st_mode))
        return SYMLINKTARGET_FILE;
    else if (S_ISDIR(sb.st_mode))
        return SYMLINKTARGET_DIRECTORY;
    else if (S_ISLNK(sb.st_mode))
    	return __GetSymbolicLinkTargetType(szTargetPath, nDepth + 1);

    return SYMLINKTARGET_OTHER;
}

static ABSTRACTOBJECTTYPE __CheckDType(uint8_t nDirType, char* szFullPath)
{
	switch (nDirType)
	{
	case DT_DIR:
		return AOT_DIRECTORY;
	case DT_REG:
		return AOT_FILE;
	case DT_UNKNOWN:
		{
			struct stat sb;
		    if (stat(szFullPath, &sb) == 0)
		    {
		        if (S_ISREG(sb.st_mode))
		        	return AOT_FILE;
		        else if (S_ISDIR(sb.st_mode))
		        	return AOT_DIRECTORY;
		    }
		}
	}

	return AOT_OTHER;
}

static char** GetDirectoryObjectsOfType(char* szPath, size_t* nCount, ABSTRACTOBJECTTYPE nObjType)
{
	*nCount = 0;

	DIR* d;
    struct dirent* dir;

    size_t nAllocatedSize = 16;
    char** lpArr = malloc(sizeof(char*) * nAllocatedSize);

    d = opendir(szPath);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
        	if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
        		continue;

        	char szFullPath[PATH_MAX];
			snprintf(szFullPath, PATH_MAX, "%s/%s", szPath, dir->d_name);

        	switch (nObjType)
        	{
        	case AOT_FILE:
        		if ((__CheckDType(dir->d_type, szFullPath) != AOT_FILE) && (__GetSymbolicLinkTargetType(szFullPath, 0) != SYMLINKTARGET_FILE))
        			continue;

        		break;
        	case AOT_DIRECTORY:
        		if ((__CheckDType(dir->d_type, szFullPath) != AOT_DIRECTORY) && (__GetSymbolicLinkTargetType(szFullPath, 0) != SYMLINKTARGET_DIRECTORY))
        			continue;

        		break;
			case AOT_OTHER:
    			assert(0);
    			break;
        	}

        	size_t nDirNameLen = strlen(szFullPath);
        	lpArr[*nCount] = malloc(nDirNameLen + 1);
        	if (!lpArr[*nCount])
        	{
        		for (int64_t n = 0l; n < *nCount; ++n)
        			free(lpArr[n]);

        		free(lpArr);
        		closedir(d);
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
        			closedir(d);
        			return NULL;
        		}

        		lpArr = lpNew;
        	}
        }

        closedir(d);
    }
    else
    	return NULL;

    return lpArr;
}

#endif