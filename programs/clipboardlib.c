#include "../bsys.h"
#include "../btools.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define CLIPBOARD_SOCK_NAME "cboard.sock"
struct clipboard {
  cstring_da elements;
  int cur;
};
int init_sock() {
  struct sockaddr_un server_addr;
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, CLIPBOARD_SOCK_NAME);
  int result =
      connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (result == 0) {
    return sock_fd;
  } else {
    return -1;
  }
}
