#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
mode_t rel_mode(mode_t c_mode, char *arg) {
  mode_t who_mask = 0;
  mode_t perm_mask = 0;
  char *p = arg;
  while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
    switch (*p) {
    case 'u':
      who_mask |= S_IRWXU;
      break;
    case 'g':
      who_mask |= S_IRWXG;
      break;
    case 'o':
      who_mask |= S_IRWXO;
      break;
    case 'a':
      who_mask |= (S_IRWXU | S_IRWXG | S_IRWXO);
      break;
    }
    p++;
  }
  if (who_mask == 0) {
    who_mask |= (S_IRWXU | S_IRWXG | S_IRWXO);
  }
  char operator = *p;
  p++;
  while (*p) {
    switch (*p) {
    case 'r':
      if (who_mask & S_IRWXU)
        perm_mask |= S_IRUSR;
      if (who_mask & S_IRWXG)
        perm_mask |= S_IRGRP;
      if (who_mask & S_IRWXO)
        perm_mask |= S_IROTH;
      break;
    case 'w':
      if (who_mask & S_IRWXU)
        perm_mask |= S_IWUSR;
      if (who_mask & S_IRWXG)
        perm_mask |= S_IWGRP;
      if (who_mask & S_IRWXO)
        perm_mask |= S_IWOTH;
      break;
    case 'x':
      if (who_mask & S_IRWXU)
        perm_mask |= S_IXUSR;
      if (who_mask & S_IRWXG)
        perm_mask |= S_IXGRP;
      if (who_mask & S_IRWXO)
        perm_mask |= S_IXOTH;
      break;
    }
    p++;
  }
  switch (operator) {
  case '+':
    return c_mode | perm_mask;
    break;
  case '-':
    return c_mode & ~perm_mask;
    break;
  case '=':
    return (c_mode & ~who_mask) | perm_mask;
    break;
  }
  return c_mode;
}
int main(int argc, char **argv) {
  if (argc == 3) {
    char *modestr = argv[1];
    char *path = argv[2];
    struct stat fstat;
    stat(path, &fstat);
    mode_t current_mode = fstat.st_mode & 07777;
    mode_t new_mode = rel_mode(current_mode, modestr);
    chmod(path, new_mode);
    exit(0);
  }
  exit(1);
}
