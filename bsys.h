#ifndef BSYS_H
#define BSYS_H
#include "btools.h"
#define CLIPBOARD_SOCK_NAME "/tmp/cboard.sock"
#define USERS_DB_PATH "/etc/users"
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
  int req_int;
};
struct login_data {
  cstring_da names;
  int cur;
};
// clipboard
int cb_init_server();
cstring *cb_get_cur();
cstring *cb_get_idx(int idx);
void cb_listen_loop(int fd, struct clipboard *clip);
struct clipboard *cb_get_all();
void cb_copy(cstring copied);
// power
void pw_reboot();
void pw_shutdown();
// sha256
cstring sha256_str_cstr(const char *input);
cstring sha256_cstr(cstring *input);
char *sha256_str(const char *input);
#endif
