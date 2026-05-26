#include "../btools.h"
#include <unistd.h>
int main() {
  char buf[4096];
  getcwd(buf, sizeof(buf));
  cprint(buf);
  cprint("\n");
  return 0;
}