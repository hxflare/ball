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
  int start;
  cstring filename;
  int rows;
  cstring *lines;
  int *dirty;
  int dirty_n;
  int *line_scroll;
};
struct static_editor_config {
  char line_char;
  int wordwrap;
};
struct runtime_data {
  int rows;
  int cols;
  int crows;
  int ccols;
  int row_ubound;
  int row_dbound;
  int col_lbound;
  int col_rbound;
  int c_tab;
  char mode;
  struct tab *tabs;
  int tabs_n;
  struct static_editor_config staticconf;
  struct termios orig_termios;
};
struct runtime_data run_data;

// functionality
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(run_data.orig_termios));
  cprint("\x1b[H\x1b[K");
  cprint("\e[1;1H\e[2J");
  cprint("exited\n");
}
void enableRawMode() {
  atexit(disableRawMode);
  struct termios raw = run_data.orig_termios;
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
    run_data.ccols--;
    break;
  case 'w':
    run_data.crows--;
    break;
  case 'd':
    run_data.ccols++;
    break;
  case 's':
    run_data.crows++;
    break;
  }
  if (run_data.crows > run_data.row_dbound - 1) {
    run_data.crows = run_data.row_dbound - 1;
    run_data.tabs[run_data.c_tab].start++;
  }
  if (run_data.ccols > run_data.col_rbound - 1) {
    run_data.ccols = run_data.col_rbound - 1;
  }
  if (run_data.crows < run_data.row_ubound) {
    run_data.crows = run_data.row_ubound;
    run_data.tabs[run_data.c_tab].start--;
  }
  if (run_data.ccols < run_data.col_lbound) {
    run_data.ccols = run_data.col_lbound;
  }
  if (run_data.tabs[run_data.c_tab].start < 0)
    run_data.tabs[run_data.c_tab].start = 0;
  int max_start = run_data.tabs[run_data.c_tab].rows - run_data.rows;
  if (max_start < 0)
    max_start = 0;
  if (run_data.tabs[run_data.c_tab].start > max_start)
    run_data.tabs[run_data.c_tab].start = max_start;
}
void process() {
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return;
  switch (c) {
  case CK('x'):
    disableRawMode();
    exit(0);
  case CK('w'):
  case CK('s'):
  case CK('a'):
  case CK('d'):
    cmove(UCK(c));
    break;
  case CK('i'):
    if (run_data.c_tab < run_data.tabs_n - 1) {
      run_data.c_tab++;
    } else {
      run_data.c_tab = 0;
    }
    break;
  }
}
void draw_lines(cstring *ab, int *rows_left, int *row) {
  int max_idxlen = intlen(run_data.tabs[run_data.c_tab].rows);
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
  for (int lineidx = ctab->start; lineidx < ctab->rows && *rows_left > 0;
       lineidx++) {
    cstring full = CSTRING_INIT;
    cstring idxstr = CSTRING_INIT;
    int_to_cstr(lineidx, &idxstr);
    for (int i = idxstr.len; i < max_idxlen; i++) {
      cchstr_append(&idxstr, ' ');
    }
    ccstr_append(&full, &idxstr);
    cchstr_append(&full, ' ');
    cchstr_append(&full, run_data.staticconf.line_char);
    cchstr_append(&full, ' ');
    ccstr_append(&full, &(ctab->lines[lineidx]));
    cstr_free(&idxstr);
    if (full.len > (unsigned)run_data.cols) {
      if (!run_data.staticconf.wordwrap) {
        int av_cols = run_data.cols - 3 - max_idxlen -4;
        cstring truncated = CSTRING_INIT;
        getrange(&(ctab->lines[lineidx]), av_cols, ctab->line_scroll[lineidx],
                 &truncated);
        ccstr_append(&(ab[*row]), &idxstr);
        cchstr_append(&(ab[*row]), ' ');
        cchstr_append(&(ab[*row]), run_data.staticconf.line_char);
        cchstr_append(&(ab[*row]), ' ');
        cchstr_append(&(ab[*row]), '<');
        ccstr_append(&(ab[*row]), &truncated);
        cchstr_append(&(ab[*row]), '>');
        cstr_free(&truncated);
      } else {
        int div = full.len / run_data.cols;
        for (int cd = 0; cd < div && *rows_left > 0; cd++) {
          cstring frac = CSTRING_INIT;
          getrange(&full, run_data.cols, cd * run_data.cols, &frac);
          ccstr_append(&(ab[*row]), &frac);
          cstr_free(&frac);
          (*rows_left)--;
          (*row)++;
        }
        int remainder = full.len % run_data.cols;
        if (remainder > 0 && *rows_left > 0) {
          cstring frac = CSTRING_INIT;
          getrange(&full, remainder, div * run_data.cols, &frac);
          ccstr_append(&(ab[*row]), &frac);
          cstr_free(&frac);
          (*rows_left)--;
          (*row)++;
        }
      }
    } else {
      ccstr_append(&(ab[*row]), &full);
      (*rows_left)--;
      (*row)++;
    }
    cstr_free(&full);
  }
}
void draw_top(cstring *ab, int *rows_left, int *row) {
  cstring full = CSTRING_INIT;
  for (int i = 0; i < run_data.tabs_n; i++) {
    if (i == run_data.c_tab) {
      cpstr_append(&full, "-->", 3);
    } else {
      cpstr_append(&full, "   ", 3);
    }
    ccstr_append(&full, &(run_data.tabs[i].filename));
    cpstr_append(&full, "   ", 3);
  }
  if (full.len > (unsigned)run_data.cols) {
    int div = full.len / run_data.cols;
    for (int cd = 0; cd < div && *rows_left > 0; cd++) {
      cstring frac = CSTRING_INIT;
      getrange(&full, run_data.cols, cd * run_data.cols, &frac);
      ccstr_append(&(ab[*row]), &frac);
      cstr_free(&frac);
      (*rows_left)--;
      (*row)++;
    }
    int remainder = full.len % run_data.cols;
    if (remainder > 0 && *rows_left > 0) {
      cstring frac = CSTRING_INIT;
      getrange(&full, remainder, div * run_data.cols, &frac);
      ccstr_append(&(ab[*row]), &frac);
      cstr_free(&frac);
      (*rows_left)--;
      (*row)++;
    }
  } else {
    if (*rows_left > 0) {
      ccstr_append(&(ab[*row]), &full);
      (*rows_left)--;
      (*row)++;
    }
  }
  cstr_free(&full);
  run_data.row_ubound = *row;
}
void merge_lines(cstring *ab, cstring *lines) {
  for (int i = 0; i < run_data.rows; i++) {
    ccstr_append(ab, &(lines[i]));
    if (i != run_data.rows - 1) {
      cpstr_append(ab, "\r\n", 2);
      cpstr_append(ab, "\x1b[K", 3);
    }
  }
}
void refresh() {
  if (run_data.tabs_n == 0)
    return;
  run_data.col_rbound = run_data.cols;
  if (run_data.tabs[run_data.c_tab].rows < run_data.rows) {
    run_data.row_dbound = run_data.tabs[run_data.c_tab].rows + 1;
  } else {
    run_data.row_dbound = run_data.rows;
  }
  int max_idxlen = intlen(run_data.tabs[run_data.c_tab].rows);
  run_data.col_lbound = 3 + max_idxlen;

  int left = run_data.rows;
  cstring ab = CSTRING_INIT;
  cstring *lines_b = malloc(sizeof(cstring) * run_data.rows);
  int row = 0;
  for (int i = 0; i < run_data.rows; i++) {
    lines_b[i] = (cstring)CSTRING_INIT;
  }
  cpstr_append(&ab, "\x1b[?25l", 6);
  cpstr_append(&ab, "\x1b[H", 3);
  draw_top(lines_b, &left, &row);
  draw_lines(lines_b, &left, &row);
  merge_lines(&ab, lines_b);

  char posbuf[32];
  snprintf(posbuf, sizeof(posbuf), "\x1b[%d;%dH", run_data.crows + 1,
           run_data.ccols + 1);
  cpstr_append(&ab, posbuf, strlen(posbuf));
  cpstr_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.str, ab.len);
  cmove('n');
  cstr_free(&ab);
  for (int i = 0; i < run_data.rows; i++) {
    cstr_free(&(lines_b[i]));
  }
  free(lines_b);
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
  run_data.tabs_n += 1;
  run_data.tabs = realloc(run_data.tabs, sizeof(struct tab) * run_data.tabs_n);
  struct tab *c_tab = &(run_data.tabs[run_data.tabs_n - 1]);
  memset(c_tab, 0, sizeof(struct tab));
  (c_tab->filename) = (cstring)CSTRING_INIT;
  cpstr_append(&(c_tab->filename), filename, strlen(filename));
  read_lines(c_tab->filename, c_tab);
  c_tab->line_scroll = calloc(c_tab->rows, sizeof(int));
  return 0;
}
// init
void initconf() {
  gwinsize(&(run_data.rows), &(run_data.cols));
  run_data.col_lbound = 3;
  run_data.col_rbound = run_data.cols;
  run_data.row_ubound = 0;
  run_data.row_dbound = run_data.rows;
  run_data.crows = run_data.row_ubound;
  run_data.ccols = run_data.col_lbound;
  run_data.tabs_n = 0;
  run_data.staticconf.line_char = '~';
  run_data.staticconf.wordwrap = 0;
}
int main(int argc, char **argv) {
  tcgetattr(STDIN_FILENO, &(run_data.orig_termios));
  cprint("\e[1;1H\e[2J");
  enableRawMode();
  initconf();
  for (int i = 1; i < argc; i++) {
    opentab(argv[i]);
  }
  while (1) {
    gwinsize(&(run_data.rows), &(run_data.cols));
    refresh();
    process();
  }
  return 0;
}
