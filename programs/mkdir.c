#include "../btools.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int main(int argc, char **argv) {
  struct stat st = {0};
  if (argc >= 2) {
    for (int i = 0; i < argc; i++) {
      if (stat(argv[i], &st) == -1) {
        mkdir(argv[i], 0700);
      } else {
        cprint("Directory ");
        cprint(argv[i]);
        cprint(" already exists\n");
      }
    }
  } else {
    cprint("Not enough arguments\n");
  }
}
