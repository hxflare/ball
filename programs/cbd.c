#include "../bsys.h"
#include "../btools.h"
#include <unistd.h>
int main(int argc, char **argv) {
  struct clipboard cb = CSTRING_DA_INIT;
  cb_listen_loop(cb_init_server(),&cb);
}
