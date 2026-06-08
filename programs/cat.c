#include "../btools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    return 0;
  }
  for (int i = 1; i < argc; i++) {
    char *dpath = argv[i];
    FILE *fptr = fopen(dpath, "r");
    if (!fptr) {
      cprint("shi... failed to open file\n");
      continue;
    }
    char *fullstr = NULL;
    char ch;
    int index = 0;
    while ((ch = fgetc(fptr)) != EOF) {
      char *tmp = realloc(fullstr, index + 2);
      if (!tmp) {
        cprint("dementia\n");
        if (fullstr)
          free(fullstr);
        fclose(fptr);
        return 1;
      }
      fullstr = tmp;
      fullstr[index++] = ch;
    }
    if (fullstr) {
      fullstr[index] = '\0';
      cprint(fullstr);
      free(fullstr);
    }

    fclose(fptr);
  }
  return 0;
}
