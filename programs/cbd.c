#include "../bsys.h"
#include "../btools.h"
#include <unistd.h>
int main(int argc, char **argv) {
  setuid(0);
  struct clipboard cb;
  cb.elements = (cstring_da)CSTRING_DA_INIT;
  cb.cur = 0;
  int fd = cb_init_server();
  while (1) {
    cb_listen_loop(fd, &cb);
  }
}
