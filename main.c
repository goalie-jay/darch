#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

#include "archiver.h"

typedef enum
{
    OP_EXTRACT,
    OP_ARCHIVE,
    OP_UNDEFINED
} MODE;

void PrintUsage()
{
    puts("Usage: darch -a <object 1> [more objects ...] -o <output>.");
}

int main(int argc, char** argv)
{
    char verbose = 0;
    MODE mode = OP_UNDEFINED;
    char* output = NULL;
    int allocatedObjs = 20;
    int objectCount = 0;
    char** objects = malloc(sizeof(char*) * allocatedObjs);

    for (int i = 1; i < argc; ++i)
    {
        char* current = argv[i];
        if (strcmp(current, "-o") == 0)
        {
            ++i;
            if (i >= argc)
            {
                PrintUsage();
                free(objects);
                return ERROR_BAD_ARGUMENTS;
            }

            output = argv[i];
        }
        else if (strcmp(current, "-v") == 0) verbose = 1;
        else if (strcmp(current, "-a") == 0) mode = OP_ARCHIVE;
        else if (strcmp(current, "-x") == 0) mode = OP_EXTRACT;
        else
        {
            if (objectCount + 1 >= allocatedObjs)
            {
                allocatedObjs *= 2;
                objects = realloc(objects, sizeof(char*) * allocatedObjs);
            }

            objects[objectCount] = current;
            ++objectCount;
        }
    }

    if (output == NULL || objectCount < 1 || mode == OP_UNDEFINED)
    {
        PrintUsage();
        free(objects);
        return ERROR_BAD_ARGUMENTS;
    }

    if (mode == OP_EXTRACT && objectCount != 1)
    {
        puts("Exactly one input file is allowed for extraction.");
        free(objects);
        return ERROR_BAD_ARGUMENTS;
    }

    switch (mode)
    {
        case OP_EXTRACT:
        {
            FILE* f = fopen(objects[0], "rb");
            if (!f)
            {
                puts("Could not open input file for reading.");
                free(objects);
                return ERROR_FILE_ACCESS;
            }

            ARCHIVER_Extract(f, output);
            fclose(f);
            break;
        }
        case OP_ARCHIVE:
        {
            FILE* f = fopen(output, "wb");
            if (!f)
            {
                puts("Could not open output file for writing.");
                free(objects);
                return ERROR_FILE_ACCESS;
            }

            ARCHIVER_Archive(objects, objectCount, f);
            fclose(f);
        }
        case OP_UNDEFINED:
            return ERROR_WTF;
    }

    free(objects);
    return 0;
}