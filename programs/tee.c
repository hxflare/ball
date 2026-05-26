#include "../btools.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char **argv) {
  int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
      perror("openerror");
    cprint("file aint opening\n");
    return 1;
  }
  write(fd, argv[2], strlen(argv[2]));
  close(fd);
  return 0;
}
