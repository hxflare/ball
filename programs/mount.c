#include "../btools.h"
#include <sys/mount.h>
int main(int argc, char **argv) {
  if (argc < 3) {
    return 1;
  }

  char *source = argv[1];
  char *target = argv[2];
  char *fstype = argc > 3 ? argv[3] : "ext4";
  char *options = argc > 4 ? argv[4] : "";

  if (mount(source, target, fstype, 0, options) == -1) {
    cprint("mount shat itself");
    return 1;
  }

  cprint("Mounted ");
  cprint(source);
  cprint(" at ");
  cprint(target);
  cprint("\n");
  return 0;
}