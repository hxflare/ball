#include "btools.h"
#ifndef BSYS_H
#define BSYS_H
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
#endif
