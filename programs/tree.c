#include "../btools.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARGIN 40
#define MAX_DEPTH 256

static const char *type_color(unsigned char type) {
  switch (type) {
  case DT_DIR:
    return "\033[33m";
  case DT_REG:
    return "\033[34m";
  case DT_BLK:
    return "\033[32m";
  case DT_LNK:
    return "\033[36m";
  case DT_FIFO:
    return "\033[35m";
  default:
    return "\033[0m";
  }
}

static const char *type_name(unsigned char type) {
  switch (type) {
  case DT_DIR:
    return "Directory";
  case DT_REG:
    return "File";
  case DT_BLK:
    return "Block Device";
  case DT_LNK:
    return "Symlink";
  case DT_FIFO:
    return "FIFO";
  case DT_SOCK:
    return "Unix Domain Socket";
  case DT_CHR:
    return "Character Device";
  default:
    return "Unknown";
  }
}

typedef struct {
  int show_colors;
  int show_filetypes;
  int show_hidden;
  int show_labels;
  int max_stage;
  int show_decorations;
} opts;

void scan_dir(const char *path, const opts *opts, int depth,
              int prefix[MAX_DEPTH]) {
  DIR *dir = opendir(path);
  if (!dir)
    return;

  struct dirent *entries[4096];
  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL && count < 4096) {
    if (entry->d_name[0] == '.' && !opts->show_hidden)
      continue;
    entries[count] = malloc(sizeof(struct dirent));
    memcpy(entries[count], entry, sizeof(struct dirent));
    count++;
  }
  closedir(dir);

  for (int i = 0; i < count; i++) {
    entry = entries[i];
    const char *name = entry->d_name;
    unsigned char type = entry->d_type;
    int is_last = (i == count - 1);

    if (opts->show_colors)
      cprint(type_color(type));
    if (opts->show_decorations) {

      for (int d = 0; d < depth; d++)
        cprint(prefix[d] ? "│   " : "    ");

      if (depth > 0)
        cprint(is_last ? "└── " : "├── ");
    }
    cprint(name);

    if (opts->show_filetypes) {
      int pad = MARGIN - (int)strlen(name);
      for (int p = 0; p < pad; p++)
        cprint(" ");
      if (opts->show_labels)
        cprint("  Type:    ");
      cprint(type_name(type));
    }

    cprint("\n");

    if (type == DT_DIR && strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
        depth < MAX_DEPTH - 1 &&
        (opts->max_stage == -1 || depth < opts->max_stage)) {
      char npath[4096];
      snprintf(npath, sizeof(npath), "%s/%s", path, name);
      prefix[depth] = !is_last;
      scan_dir(npath, opts, depth + 1, prefix);
      prefix[depth] = 0;
    }

    free(entries[i]);
  }

  if (opts->show_colors)
    cprint("\033[0m");
}

int main(int argc, char **argv) {
  opts opts = {
      .show_hidden = 0,
      .show_filetypes = 0,
      .show_colors = 0,
      .show_labels = 0,
      .max_stage = -1,
      .show_decorations = 1,
  };
  const char *path = ".";

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (str_isdigit(argv[i]))
        opts.max_stage = atoi(argv[i]);
      else
        path = argv[i];
    } else {
      for (int k = 1; argv[i][k]; k++) {
        switch (argv[i][k]) {
        case 'h':
          opts.show_hidden = 1;
          break;
        case 't':
          opts.show_filetypes = 1;
          break;
        case 'c':
          opts.show_colors = 1;
          break;
        case 'd':
          opts.show_decorations = 0;
          break;
        case 'i':
          opts.show_labels = 1;
          break;
        case 's':
          if (i + 1 < argc && str_isdigit(argv[i + 1]))
            opts.max_stage = atoi(argv[++i]);
          else
            cprint("shi... -s requires a number argument\n");
          break;
        default:
          cprint("shi... invalid flag -");
          cprint((char[]){argv[i][k], 0});
          cprint("\n");
          break;
        }
      }
    }
  }

  DIR *check = opendir(path);
  if (!check) {
    cprint("shi... unable to open directory\n");
    return EXIT_FAILURE;
  }
  closedir(check);

  int prefix[MAX_DEPTH] = {0};
  scan_dir(path, &opts, 0, prefix);
  return EXIT_SUCCESS;
}