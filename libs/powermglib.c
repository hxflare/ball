#include "../bsys.h"
#include <sys/reboot.h>
#include <unistd.h>
void pw_reboot() {
  setuid(0);
  sync();
  reboot(RB_AUTOBOOT);
}
void pw_shutdown() {
  setuid(0);
  sync();
  reboot(RB_POWER_OFF);
}
