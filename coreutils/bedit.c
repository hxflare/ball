#include "../btools.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// defines
#define CK(k) ((k) & 0x1f)
#define UCK(k) ((k) | 0x60)
// data structures
struct tab {
  cstring filename;
  int rows;
  cstring *lines;
  int *dirty;
  int dirty_n;
};
struct static_editor_config {
  char line_char;
};
struct editor_config {
  int rows;
  int cols;
  int crows;
  int ccols;
  int row_ubound;
  int row_dbound;
  int col_lbound;
  int col_rbound;
  int start;
  int c_tab;
  char mode;
  struct tab *tabs;
  int tabs_n;
  struct static_editor_config staticconf;
  struct termios orig_termios;
};
struct editor_config config;

// functionality
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(config.orig_termios));
  cprint("\x1b[H\x1b[K");
  cprint("exited\n");
}
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &(config.orig_termios));
  atexit(disableRawMode);
  struct termios raw = config.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_cflag |= (CS8);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void cmove(char key) {
  switch (key) {
  case 'a':
    config.ccols--;
    break;
  case 'w':
    config.crows--;
    break;
  case 'd':
    config.ccols++;
    break;
  case 's':
    config.crows++;
    break;
  }
  if (config.crows > config.row_dbound - 1) {
    config.crows = config.row_dbound - 1;
    config.start++;
  }
  if (config.ccols > config.col_rbound - 1) {
    config.ccols = config.col_rbound - 1;
  }
  if (config.crows < config.row_ubound) {
    config.crows = config.row_ubound;
    config.start--;
  }
  if (config.ccols < config.col_lbound) {
    config.ccols = config.col_lbound;
  }
  if (config.start < 0) {
    config.start = 0;
  }
  if (config.start > config.tabs[config.c_tab].rows - config.rows) {
    config.start = config.tabs[config.c_tab].rows - config.rows;
  }
}
void process() {
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return;
  switch (c) {
  case CK('x'):
    exit(0);
  case CK('w'):
  case CK('s'):
  case CK('a'):
  case CK('d'):
    cmove(UCK(c));
    break;
  case CK('i'):
    if (config.c_tab < config.tabs_n - 1) {
      config.c_tab++;
    } else {
      config.c_tab = 0;
    }
    break;
  }
}
void draw_lines(cstring *ab, int *rows_left) {
  int bound = *rows_left - 1;
  cpstr_append(ab, "\r\n", 2);
  int max_idxlen = intlen(config.tabs[config.c_tab].rows);
  for (int y = 0; y < bound; y++) {
    cstring *c_line = &(config.tabs[config.c_tab].lines[y + config.start]);
    int t_offset = 0;
    cstring str_idx = CSTRING_INIT;
    int_to_cstr(y + config.start, &str_idx);
    t_offset += str_idx.len + 1;
    ccstr_append(ab, &str_idx);
    for (int i = 0; i < max_idxlen - intlen(y + config.start); i++) {
      cchstr_append(ab, ' ');
    }
    cchstr_append(ab, ' ');
    cchstr_append(ab, config.staticconf.line_char);
    cchstr_append(ab, ' ');
    t_offset += 3;
    config.col_lbound = t_offset;
    cpstr_append(ab, "\x1b[K", 3);
    if (y < config.tabs[config.c_tab].rows) {
      ccstr_append(ab, c_line);
      (*rows_left) -= (c_line->len + t_offset) / config.rows;
    }
    if (y < bound - 1) {
      cpstr_append(ab, "\r\n", 2);
      (*rows_left)--;
    }
  }
}
void draw_top(cstring *ab, int *rows_left) {
  int accumulated_len = 0;
  for (int i = 0; i < config.tabs_n; i++) {
    if (i == config.c_tab) {
      cpstr_append(ab, " -->", 4);
    } else {
      cpstr_append(ab, "    ", 4);
    }
    ccstr_append(ab, &(config.tabs[i].filename));
    accumulated_len += 8 + config.tabs[i].filename.len;
  }
  cpstr_append(ab, "\x1b[K", 3);
  (*rows_left) -= accumulated_len / config.cols;
  config.row_ubound = config.rows - *rows_left + 1;
}
void refresh() {
  config.col_rbound = config.cols;
  config.row_dbound = config.rows;

  int left = config.rows;
  cstring ab = CSTRING_INIT;
  cpstr_append(&ab, "\x1b[?25l", 6);
  cpstr_append(&ab, "\x1b[H", 3);
  draw_top(&ab, &left);
  draw_lines(&ab, &left);
  char posbuf[32];
  snprintf(posbuf, sizeof(posbuf), "\x1b[%d;%dH", config.crows + 1,
           config.ccols + 1);
  cpstr_append(&ab, posbuf, strlen(posbuf));
  cpstr_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.str, ab.len);
  cmove('n');
  cstr_free(&ab);
}
void read_lines(cstring filename, struct tab *out) {
  int fd = open(filename.str, O_CREAT | O_RDWR, 0644);
  char c;
  int line_idx = 0;
  out->lines = realloc(out->lines, sizeof(cstring) * (line_idx + 1));
  out->lines[line_idx] = (cstring)CSTRING_INIT;
  while (read(fd, &c, 1) == 1) {
    if (c == '\n') {
      line_idx += 1;
      out->lines = realloc(out->lines, sizeof(cstring) * (line_idx + 1));
      out->lines[line_idx] = (cstring)CSTRING_INIT;
      continue;
    }
    cchstr_append(&(out->lines[line_idx]), c);
  }
  out->rows = line_idx + 1;
  close(fd);
}
int opentab(char *filename) {
  config.tabs_n += 1;
  config.tabs = realloc(config.tabs, sizeof(struct tab) * config.tabs_n);
  struct tab *c_tab = &(config.tabs[config.tabs_n - 1]);
  (c_tab->filename) = (cstring)CSTRING_INIT;
  cpstr_append(&(c_tab->filename), filename, strlen(filename));
  read_lines(c_tab->filename, c_tab);
  return 0;
}
// init
void initconf() {
  gwinsize(&(config.rows), &(config.cols));
  config.start = 0;
  config.col_lbound = 3;
  config.col_rbound = config.cols;
  config.row_ubound = 0;
  config.row_dbound = config.rows;
  config.crows = config.row_ubound;
  config.ccols = config.col_lbound;
  config.tabs_n = 0;
  config.staticconf.line_char = '~';
}
int main(int argc, char **argv) {
  enableRawMode();
  initconf();
  for (int i = 1; i < argc; i++) {
    opentab(argv[i]);
  }
  while (1) {
    gwinsize(&(config.rows), &(config.cols));
    refresh();
    process();
  }
  return 0;
}
