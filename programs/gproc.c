#include "../btools.h"
#include <dirent.h>
#include <stdio.h>
int main(int argc, char **argv) {
  DIR *proc = opendir("/proc");
  struct dirent *entry;
  while ((entry = readdir(proc)) != NULL) {
    if (!str_isdigit(entry->d_name))
      continue;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
    FILE *f = fopen(path, "r");
    if (!f)
      continue;
    char name[256];
    fgets(name, sizeof(name), f);
    fclose(f);
    printf("PID: %s\tNAME: %s", entry->d_name, name);
  }
  closedir(proc);
}
