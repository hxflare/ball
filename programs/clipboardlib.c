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
enum cb_req_type {
  CHANGE_C,
  ADD_NEW,
  DEL_AT,
  INSERT_AT,
};
struct cb_request {
  enum cb_req_type type;
  cstring req_str;
  int req_int;
};
int init_server() {
  int server_socket;

  struct sockaddr_un server_addr;
  int result;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, "unix_socket");
  int slen = sizeof(server_addr);
  result = bind(server_socket, (struct sockaddr *)&server_addr, slen);
  return server_socket;
}
void start_listen(int fd) {
  int client_socket;
  struct sockaddr_un client_addr;
  listen(fd, 1);
  while (1) {
    cstring text;
  }
}
