#include "../btools.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int main(int argc, char **argv) {
  struct stat st = {0};
  if (argc >= 2) {
    if (stat(argv[argc - 1], &st) == -1) {
      mkdir(argv[argc - 1], 0700);
    } else {
      cprint("Directory ");
      cprint(argv[argc - 1]);
      cprint(" already exists\n");
    }
  }else {
    cprint("Not enough arguments\n");
  }
}