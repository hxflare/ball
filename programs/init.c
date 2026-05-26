#include "../btools.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main(void) {
  mount("none", "/proc", "proc", 0, NULL);
  mount("none", "/sys", "sysfs", 0, NULL);
  mount("none", "/dev", "devtmpfs", 0, NULL);
  mkdir("/dev/pts", 0755);
  mount("none", "/dev/pts", "devpts", 0, NULL);
  mount("none", "/tmp", "tmpfs", 0, NULL);
  setenv("PATH", "/bin:/sbin:/usr/bin:/usr/local/bin", 1);
  setenv("HOME", "/root", 1);
  setenv("TERM", "linux", 1);
  char *argv[] = {"/bin/ball", NULL};
  char *envp[] = {"PATH=/bin:/sbin:/usr/bin:/usr/local/bin", "HOME=/root",
                  "TERM=linux", "USER=root", NULL};
  pid_t forked = fork();
  if (forked == 0) {
    execve("/bin/initsys", argv, envp);
    cprint("shi.... execve failed");
  } else if (forked == -1) {
    cprint("fork failed in init\n");
  } else {
    while (1) {
      waitpid(-1, NULL, 0);
    }
  }

  reboot(RB_AUTOBOOT);
  return 1;
}