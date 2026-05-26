#include "../btools.h"
#include <string.h>
int main(int argc, char *argv[]) {
  int newline = 1;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0) {
      newline = 0;
    } else {
      cprint(argv[i]);
      if (i < argc - 1) {
        cprint(" ");
      }
    }
  }
  if (newline) {
    cprint("\n");
  }
  return 0;
}