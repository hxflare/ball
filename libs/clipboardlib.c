#include "../bsys.h"
#include "../btools.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
static void write_cstring(int fd, cstring *s) {
  int32_t len = (int32_t)s->len;
  write(fd, &len, sizeof(len));
  if (len > 0)
    write(fd, s->str, len);
}
static cstring read_cstring(int fd) {
  cstring s = CSTRING_INIT;
  int32_t len = 0;
  read(fd, &len, sizeof(len));
  if (len > 0) {
    s.str = malloc(len);
    s.len = len;
    int received = 0;
    while (received < len) {
      int r = read(fd, s.str + received, len - received);
      if (r <= 0)
        break;
      received += r;
    }
  }
  return s;
}
int connect_cb_sock() {
  int server_socket;
  struct sockaddr_un server_addr;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, CLIPBOARD_SOCK_NAME);
  int res = connect(server_socket, (struct sockaddr *)&server_addr,
                    sizeof(server_addr));
  return server_socket;
}
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
    write_cstring(fd, &(clip->elements.strs[request->req_int]));
    break;
  case GET_CUR:
    write_cstring(fd, &(clip->elements.strs[clip->cur]));
    break;
  case GET_ALL:
    int32_t n = clip->elements.n;
    write(fd, &n, sizeof(n));
    for (int i = 0; i < n; i++)
      write_cstring(fd, &clip->elements.strs[i]);
    break;
  case ADD_NEW:
    csta_insert(&(clip->elements), &(request->req_str), 0);
    break;
  case INSERT_AT:
    csta_insert(&(clip->elements), &(request->req_str), request->req_int);
    break;
  case CHANGE_CUR_IDX:
    clip->cur = request->req_int;
    break;
  case DEL_AT:
    csta_pop(&(clip->elements), request->req_int);
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
    handle_request(&request_data, client_socket, clip);
    close(client_socket);
  }
}
cstring *cb_get_idx(int idx) {
  cstring *s_str = calloc(1, sizeof(cstring));
  struct cb_request request;
  int server_socket = connect_cb_sock();
  request.req_int = idx;
  request.req_str = (cstring)CSTRING_INIT;
  request.type = GET_AT;
  write(server_socket, &request, sizeof(request));
  read(server_socket, s_str, sizeof(cstring));
  return s_str;
}
cstring *cb_get_cur() {
  cstring *s_str = calloc(1, sizeof(cstring));
  struct cb_request request;
  int server_socket = connect_cb_sock();
  request.req_int = 0;
  request.req_str = (cstring)CSTRING_INIT;
  request.type = GET_CUR;
  write(server_socket, &request, sizeof(request));
  read(server_socket, s_str, sizeof(cstring));
  return s_str;
}
struct clipboard *cb_get_all() {
  struct clipboard *cb = calloc(1, sizeof(struct clipboard));
  struct cb_request request;
  int server_socket = connect_cb_sock();
  request.req_int = 0;
  request.req_str = (cstring)CSTRING_INIT;
  request.type = GET_ALL;
  write(server_socket, &request, sizeof(request));
  read(server_socket, cb, sizeof(struct clipboard));
  return cb;
}
void cb_copy(cstring copied) {
  int server_socket = connect_cb_sock();
  struct cb_request request;
  request.type = ADD_NEW;
  request.req_int = 0;
  request.req_str = copied;
  write(server_socket, &request, sizeof(request));
}
