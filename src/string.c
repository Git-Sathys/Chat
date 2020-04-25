#include <stdio.h>
#include "string.h"

void str_trim_lf (char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // remplace le \n de la saisie par un \0
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void str_overwrite_stdout(char name[]) {
    printf("\r %s > ", name);
    fflush(stdout);
}
