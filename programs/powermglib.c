#include "../bsys.h"
#include <sys/reboot.h>
#include <unistd.h>
void pw_reboot() {
  sync();
  setuid(0);
  reboot(RB_AUTOBOOT);
}
void pw_shutdown() {
  sync();
  setuid(0);
  reboot(RB_POWER_OFF);
}
