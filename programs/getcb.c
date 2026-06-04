#include "../bsys.h"
#include "../btools.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char **argv) {
  cstring *cb = cb_get_cur();
  write(1, cb->str, cb->len);
}
