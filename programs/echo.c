#include "../btools.h"
#include <string.h>
#include <unistd.h>

static void print_unescaped(const char *s) {
  while (*s) {
    if (s[0] == '\\' && s[1] != '\0') {
      s++;
      switch (*s) {
      case 'e':
        write(1, "\x1b", 1);
        break;
      case 'n':
        write(1, "\n", 1);
        break;
      case 't':
        write(1, "\t", 1);
        break;
      case 'r':
        write(1, "\r", 1);
        break;
      case '\\':
        write(1, "\\", 1);
        break;
      default:
        write(1, "\\", 1);
        write(1, s, 1);
        break;
      }
    } else {
      write(1, s, 1);
    }
    s++;
  }
}
int main(int argc, char **argv) {
  int newline = 1;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0) {
      newline = 0;
    } else {
      print_unescaped(argv[i]);
      if (i < argc - 1) {
        cprint(" ");
      }
    }
  }
  if (newline) {
    cprint("\n");
  }
  setcol(reset, reset);
  return 0;
}
