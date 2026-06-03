
#ifndef BSYS_H
#define BSYS_H
#include "btools.h"
#define CLIPBOARD_SOCK_NAME "cboard.sock"
struct clipboard {
  cstring_da elements;
  int cur;
};
enum cb_req_type {
  CHANGE_CUR_IDX,
  ADD_NEW,
  DEL_AT,
  INSERT_AT,
  GET_AT,
  GET_ALL,
  GET_CUR,
};
struct cb_request {
  enum cb_req_type type;
  cstring req_str;
  int req_int;
};
int cb_init_server();
void cb_start_listen_loop(int fd);
cstring *cb_get_cur();
cstring *cb_get_idx(int idx);
void cb_listen_loop(int fd, struct clipboard *clip);
struct clipboard *cb_get_all();
void cb_copy(cstring copied);
void pw_reboot();
void pw_shutdown();
#endif
