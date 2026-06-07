#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
int main(int argc, char **argv) {
  if (argc == 3) {
    char *modestr = argv[1];
    char *path = argv[2];
    char *endptr;
    mode_t mode = (mode_t)(strtol(modestr, &endptr, 8));
    chmod(path, mode);
    exit(0);
  }
  exit(1);
}
