#include "../bsys.h"
#include "../btools.h"
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
  if (argc <= 1) {
    exit(EXIT_FAILURE);
  }
  cstring full = CSTRING_INIT;
  for (int i = 1; i < argc; i++) {
    cpstr_append(&full, argv[i], strlen(argv[i]));
  }
  cb_copy(full);
}
