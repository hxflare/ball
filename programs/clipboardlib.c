#include "../bsys.h"
#include "../btools.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int cb_init_server() {
  int server_socket;
  struct sockaddr_un server_addr;
  int result;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, CLIPBOARD_SOCK_NAME);
  socklen_t slen = sizeof(server_addr);
  unlink(CLIPBOARD_SOCK_NAME);
  result = bind(server_socket, (struct sockaddr *)&server_addr, slen);
  return server_socket;
}
void handle_request(struct cb_request *request, int fd,
                    struct clipboard *clip) {
  switch (request->type) {
  case GET_AT:
    write(fd, &(clip->elements.strs[request->req_int]),
          sizeof(request->req_str));
    break;
  case GET_CUR:
    write(fd, &(clip->elements.strs[(*clip).cur]), sizeof(request->req_str));
    break;
  case GET_ALL:
    write(fd, clip, sizeof(*clip));
    break;
  case ADD_NEW:
    csta_append(&(clip->elements), &(request->req_str));
    break;
  }
}
void cb_listen_loop(int fd, struct clipboard *clip) {
  int client_socket;
  struct sockaddr_un client_addr;
  listen(fd, 5);
  while (1) {
    struct cb_request request_data;
    socklen_t clen = sizeof(client_addr);
    client_socket = accept(fd, (struct sockaddr *)&client_addr, &clen);
    read(client_socket, &request_data, sizeof(struct cb_request));
    handle_request(&request_data, fd, clip);
  }
}
void *cb_get(int idx) {
  cstring *s_str;
  struct cb_request request;
  int server_socket;
  struct sockaddr_un server_addr;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, CLIPBOARD_SOCK_NAME);
  int res = connect(server_socket, (struct sockaddr *)&server_addr,
                    sizeof(server_addr));
  request.req_int = idx;
  request.req_str = (cstring)CSTRING_INIT;
  if (idx >= 0) {
    request.type = GET_AT;
    write(server_socket, &request, sizeof(request));
    read(server_socket, s_str, sizeof(cstring));
    return s_str;
  } else if (idx == -2) {
    struct clipboard *cb;
    request.type = GET_ALL;
    write(server_socket, &request, sizeof(request));
    read(server_socket, cb, sizeof(struct clipboard));
    return cb;
  } else if (idx == -1) {
    request.type = GET_CUR;
    write(server_socket, &request, sizeof(request));
    read(server_socket, s_str, sizeof(cstring));
    return s_str;
  }
  return NULL;
}
void cb_copy(cstring copied) {
  struct cb_request request;

  int server_socket;
  struct sockaddr_un server_addr;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, CLIPBOARD_SOCK_NAME);
  int res = connect(server_socket, (struct sockaddr *)&server_addr,
                    sizeof(server_addr));
  request.type = ADD_NEW;
  request.req_int = 0;
  request.req_str = copied;
  write(server_socket, &request, sizeof(request));
}
