#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../btools.h"
int main(int argc, char **argv) {
    char *dpath =argv[1];
    if (dpath[0]=='/'){
        char *path=malloc(strlen(dpath)+3);
        snprintf(path, strlen(path)+3, "./%s", dpath);
    }else {
        char *path=dpath;
    }
    char *fullstr = NULL;
    char ch;
    FILE *fptr = fopen(argv[1], "r");
    if (!fptr) {
        cprint("shi... failed to open file");
        return 1;
    }
    int index = 0;
    while ((ch = fgetc(fptr)) != EOF) {
        fullstr = realloc(fullstr, index + 2);
        if (!fullstr) {
            cprint("dementia");
            fclose(fptr);
            return 1;
        }
        fullstr[index++] = ch;
    }
    if (fullstr)
        fullstr[index] = '\0';
    if (fullstr)
        cprint(fullstr);
    free(fullstr);
    fclose(fptr);
    return 0;
}
