#include "files.h"
#include <stdio.h>
#include <stdlib.h>
char* readTextFile(const char* path) { 
    FILE* file = fopen(path, "r");
    if (file == 0) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* str = malloc(size);
    fread(str, 1, size, file);

    return str;
}