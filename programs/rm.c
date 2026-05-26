#include "../btools.h"
#include <unistd.h>
int main(int argc, char **argv) {
  if (argc < 2) {
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    if (unlink(argv[i]) == -1) {
      cprint("failed to remove file ");
      cprint(argv[i]);
      cprint("\n");
    }
  }
}