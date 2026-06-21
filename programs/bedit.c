#include "../btools.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CK(k) ((k) & 0x1f)
#define UCK(k) ((k) | 0x60)
enum editor_mode {
  view,
  edit,
  settings,
  command,
};
enum cmove_type {
  left,
  right,
  down,
  up,
  ctrl_left,
  ctrl_right,
  ctrl_shift_right,
  ctrl_shift_left,
  shift_up,
  shift_down,
  shift_left,
  shift_right,
};
struct floating_win {
  int start;
  cstring *choices;
  int n;
  cstring title;
  int c_choice;
};
struct tab {
  cstring selected;
  int2 select_start;
  int2 select_end;
  int start;
  cstring filename;
  cstring_da lines;
  int2 mem_pos;
  int *line_scroll;
};
struct static_editor_config {
  char line_char;
  int wordwrap;
  int tab_style;
};
struct runtime_data {
  int rows;
  int cols;
  int2 cpos;
  int2 row_bound;
  int2 col_bound;
  int c_tab;
  struct floating_win *window;
  enum editor_mode mode;
  struct tab *tabs;
  int tabs_n;
  struct static_editor_config staticconf;
  struct termios orig_termios;
};
struct runtime_data run_data;
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(run_data.orig_termios));
  cprint("\x1b[H\x1b[K");
  cprint("\e[1;1H\e[2J");
  cprint("exited\n");
}
void switch_tabs(int tab_i) {
  struct tab *prev = &(run_data.tabs[run_data.c_tab]);
  prev->mem_pos = run_data.cpos;
  run_data.c_tab = tab_i;
  struct tab *new = &(run_data.tabs[tab_i]);
  run_data.cpos = new->mem_pos;
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
void clamp_start() {
  int max_start = run_data.tabs[run_data.c_tab].lines.n - run_data.rows;
  if (max_start < 0)
    max_start = 0;
  if (run_data.tabs[run_data.c_tab].start > max_start)
    run_data.tabs[run_data.c_tab].start = max_start;
  if (run_data.tabs[run_data.c_tab].start < 0)
    run_data.tabs[run_data.c_tab].start = 0;
}
int abs_row() {
  return run_data.tabs[run_data.c_tab].start + run_data.cpos.y -
         run_data.row_bound.x;
}
int abs_col() { return run_data.cpos.x - run_data.col_bound.x; }
void write_file() {
  cstring full = CSTRING_INIT;
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
  for (int i = 0; i < ctab->lines.n; i++) {
    ccstr_append(&full, &(ctab->lines.strs[i]));
    if (i != ctab->lines.n - 1)
      cchstr_append(&full, '\n');
  }
  char *path = malloc(ctab->filename.len + 1);
  memcpy(path, ctab->filename.str, ctab->filename.len);
  path[ctab->filename.len] = '\0';
  int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  write(fd, full.str, full.len);
  close(fd);
}
void clamp_cursor() {
  switch (run_data.mode) {
  case view:
    if (run_data.cpos.y < run_data.row_bound.x) {
      run_data.cpos.y = run_data.row_bound.x;
      if (run_data.tabs[run_data.c_tab].start > 0)
        run_data.tabs[run_data.c_tab].start--;
    } else if (run_data.cpos.y > run_data.row_bound.y - 1) {
      run_data.cpos.y = run_data.row_bound.y - 1;
      run_data.tabs[run_data.c_tab].start++;
    }
    if (run_data.cpos.x < run_data.col_bound.x) {
      run_data.cpos.x = run_data.col_bound.x;
    } else if (run_data.cpos.x > run_data.col_bound.y - 1) {
      run_data.cpos.x = run_data.col_bound.y - 1;
    }
    break;
  case edit: {
    if (run_data.cpos.y < run_data.row_bound.x) {
      run_data.cpos.y = run_data.row_bound.x;
      run_data.tabs[run_data.c_tab].start--;
    } else if (run_data.cpos.y > run_data.row_bound.y - 1) {
      run_data.cpos.y = run_data.row_bound.y - 1;
      run_data.tabs[run_data.c_tab].start++;
    }
    clamp_start();
    int ar = abs_row();
    struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
    if (ar < 0)
      ar = 0;
    if (ar >= ctab->lines.n)
      ar = ctab->lines.n - 1;
    int line_len = ctab->lines.strs[ar].len;
    if (run_data.cpos.x < run_data.col_bound.x) {
      run_data.cpos.x = run_data.col_bound.x;
    } else if (run_data.cpos.x > run_data.col_bound.x + line_len) {
      run_data.cpos.x = run_data.col_bound.x + line_len;
    }
    if (run_data.cpos.x > run_data.col_bound.y - 1) {
      run_data.cpos.x = run_data.col_bound.y - 1;
    }
    break;
  }
  default:
    break;
  }
  clamp_start();
}
char get_cur_char() {
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
  int line_idx = abs_row();
  int col_idx = abs_col();
  if (line_idx > ctab->lines.n) {
    return ' ';
  } else if (col_idx > ctab->lines.strs[line_idx].len) {
    return ' ';
  }
  return ctab->lines.strs[line_idx].str[col_idx];
}

void cmove(enum cmove_type type) {
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
  int c_abrow = run_data.cpos.y + ctab->start - run_data.row_bound.x;
  int c_abcol = run_data.cpos.x - run_data.col_bound.x;
  cstring *cline = &(ctab->lines.strs[c_abrow]);
  switch (type) {
  case left:
    run_data.cpos.x--;
    break;
  case up:
    run_data.cpos.y--;
    break;
  case right:
    run_data.cpos.x++;
    break;
  case down:
    run_data.cpos.y++;
    break;
  case ctrl_left: {
    int i_a = c_abcol - 1;
    while (cline->str[i_a] != ' ') {
      if (i_a < 0)
        break;
      i_a--;
      run_data.cpos.x--;
    }
    break;
  }
  case ctrl_right: {
    int i_d = c_abcol + 1;
    while (cline->str[i_d] != ' ') {
      if (i_d < 0)
        break;
      i_d++;
      run_data.cpos.x++;
    }
    break;
  }
  case shift_left:
    run_data.cpos.x--;
    break;
  }
  clamp_cursor();
}
void place_char(char c) {
  int c_abrow = abs_row();
  int c_abcol = run_data.cpos.x - run_data.col_bound.x;
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);

  if (c == KEY_ENTER) {
    cstring *cur_line = &ctab->lines.strs[c_abrow];
    cstring tail = CSTRING_INIT;
    if (c_abcol < cur_line->len) {
      cpstr_append(&tail, cur_line->str + c_abcol, cur_line->len - c_abcol);
      cur_line->len = c_abcol;
    }
    csta_insert(&ctab->lines, &tail, c_abrow + 1);
    ctab->line_scroll =
        realloc(ctab->line_scroll, sizeof(int) * ctab->lines.n);
    memmove(ctab->line_scroll + c_abrow + 1, ctab->line_scroll + c_abrow,
            sizeof(int) * (ctab->lines.n - c_abrow - 1));
    ctab->line_scroll[c_abrow + 1] = 0;
    run_data.cpos.y += 2;
    run_data.cpos.x = run_data.col_bound.x;
  } else if (c == KEY_BACKSPACE) {
    if (c_abcol > 0) {
      cstring *line = &ctab->lines.strs[c_abrow];
      memmove(line->str + c_abcol - 1, line->str + c_abcol,
              line->len - c_abcol);
      line->len--;
      run_data.cpos.x--;
    } else if (c_abrow > 0) {
      int prev_len = ctab->lines.strs[c_abrow - 1].len;
      cpstr_append(&ctab->lines.strs[c_abrow - 1], ctab->lines.strs[c_abrow].str,
                   ctab->lines.strs[c_abrow].len);
      cstr_free(&ctab->lines.strs[c_abrow]);
      csta_pop(&ctab->lines, c_abrow);
      memmove(ctab->line_scroll + c_abrow, ctab->line_scroll + c_abrow + 1,
              sizeof(int) * (ctab->lines.n - c_abrow));
      ctab->line_scroll =
          realloc(ctab->line_scroll, sizeof(int) * ctab->lines.n);
      run_data.cpos.y--;
      run_data.cpos.x = run_data.col_bound.x + prev_len;
    }
  } else {
    chcinsert(&(ctab->lines.strs[c_abrow]), c_abcol, c);
    cmove(right);
  }
  clamp_cursor();
}
void exit_clean() { exit(0); };
void process_input() {
  clamp_cursor();
  int key = read_key();
  if (key == KEY_NULL)
    return;
  switch (key) {
  case CK('x'):
    exit_clean();
    break;
  case KEY_ARROW_UP:
    cmove(up);
    break;
  case KEY_ARROW_DOWN:
    cmove(down);
    break;
  case KEY_ARROW_LEFT:
    cmove(left);
    break;
  case KEY_ARROW_RIGHT:
    cmove(right);
    break;
  case KEY_CTRL_ARROW_LEFT:
    cmove(ctrl_left);
    break;
  case KEY_CTRL_ARROW_RIGHT:
    cmove(ctrl_right);
    break;
  case KEY_SHIFT_ARROW_LEFT:
    cmove(shift_left);
    break;
  case KEY_SHIFT_ARROW_RIGHT:
    cmove(shift_right);
    break;
  case CK('t'):
    if (run_data.c_tab >= run_data.tabs_n - 1) {
      switch_tabs(0);
    } else {
      switch_tabs(run_data.c_tab + 1);
    }
    break;
  case CK('e'):
    run_data.mode = edit;
    break;
  case CK('b'):
    run_data.mode = view;
    break;
  case CK('s'):
    write_file();
    break;
  case CK('I'):
    for (int i = 0; i < 4; i++) {
      place_char(' ');
    }
    break;
  default:
    if (run_data.mode == edit) {
      place_char(key);
    }
    break;
  }
  clamp_cursor();
}
void draw_lines(cstring *ab, int *rows_left, int *row) {
  int max_idxlen = intlen(run_data.tabs[run_data.c_tab].lines.n);
  struct tab *ctab = &(run_data.tabs[run_data.c_tab]);
  for (int lineidx = ctab->start; lineidx < ctab->lines.n && *rows_left > 0;
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
    ccstr_append(&full, &(ctab->lines.strs[lineidx]));
    int vlen = cstrvislen(&full);
    if (run_data.cols > 2 && vlen > (unsigned)(run_data.cols - 2)) {
      if (!run_data.staticconf.wordwrap) {
        int prefix_len = max_idxlen + 4;
        int suffix_len = 1;
        int av_cols = run_data.cols - prefix_len - suffix_len;

        if (av_cols <= 0) {
          ccstr_append(&(ab[*row]), &idxstr);
          cchstr_append(&(ab[*row]), ' ');
          cchstr_append(&(ab[*row]), run_data.staticconf.line_char);
          (*rows_left)--;
          (*row)++;
        } else {
          cstring truncated = CSTRING_INIT;
          getrange(&(ctab->lines.strs[lineidx]), av_cols, ctab->line_scroll[lineidx],
                   &truncated);
          ccstr_append(&(ab[*row]), &idxstr);
          cchstr_append(&(ab[*row]), ' ');
          cchstr_append(&(ab[*row]), run_data.staticconf.line_char);
          cchstr_append(&(ab[*row]), ' ');
          cstcol(&(ab[*row]), black, white);
          cchstr_append(&(ab[*row]), '<');
          cstcol(&(ab[*row]), white, black);
          ccstr_append(&(ab[*row]), &truncated);
          cstcol(&(ab[*row]), black, white);
          cchstr_append(&(ab[*row]), '>');
          cstcol(&(ab[*row]), white, black);
          cstr_free(&truncated);
          (*rows_left)--;
          (*row)++;
        }
      } else {
        int div = vlen / run_data.cols;
        for (int cd = 0; cd < div && *rows_left > 0; cd++) {
          cstring frac = CSTRING_INIT;
          getrange(&full, run_data.cols, cd * run_data.cols, &frac);
          ccstr_append(&(ab[*row]), &frac);
          cstr_free(&frac);
          (*rows_left)--;
          (*row)++;
        }
        int remainder = vlen % run_data.cols;
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
    cstr_free(&idxstr);
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
  int vlen = cstrvislen(&full);
  if (vlen > (unsigned)run_data.cols) {
    int div = vlen / run_data.cols;
    for (int cd = 0; cd < div && *rows_left > 0; cd++) {
      cstring frac = CSTRING_INIT;
      getrange(&full, run_data.cols, cd * run_data.cols, &frac);
      ccstr_append(&(ab[*row]), &frac);
      cstr_free(&frac);
      (*rows_left)--;
      (*row)++;
    }
    int remainder = vlen % run_data.cols;
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

  run_data.row_bound.x = *row;
}
void merge_lines(cstring *ab, cstring *lines, int used_rows) {
  for (int i = 0; i < run_data.rows; i++) {
    if (i < used_rows)
      ccstr_append(ab, &(lines[i]));
    cpstr_append(ab, "\x1b[K", 3);
    if (i != run_data.rows - 1)
      cpstr_append(ab, "\r\n", 2);
  }
}
void draw_viewer() {
  if (run_data.tabs_n == 0)
    return;
  run_data.col_bound.y = run_data.cols;
  if (run_data.tabs[run_data.c_tab].lines.n < run_data.rows) {
    run_data.row_bound.y =
        run_data.tabs[run_data.c_tab].lines.n + run_data.row_bound.x;
  } else {
    run_data.row_bound.y = run_data.rows;
  }
  int max_idxlen = intlen(run_data.tabs[run_data.c_tab].lines.n);
  run_data.col_bound.x = 3 + max_idxlen;
  int left = run_data.rows;
  cstring ab = CSTRING_INIT;
  cstring *lines_b = malloc(sizeof(cstring) * run_data.rows);
  int row = 0;
  for (int i = 0; i < run_data.rows; i++) {
    lines_b[i] = (cstring)CSTRING_INIT;
  }
  cpstr_append(&ab, "\x1b[?25l", 6);
  cpstr_append(&ab, "\x1b[H", 3);

  run_data.row_bound.x = 0;
  if (run_data.staticconf.tab_style) {
    draw_top(lines_b, &left, &row);
  }

  if (run_data.tabs[run_data.c_tab].lines.n <
      run_data.rows - run_data.row_bound.x) {
    run_data.row_bound.y =
        run_data.tabs[run_data.c_tab].lines.n + run_data.row_bound.x;
  } else {
    run_data.row_bound.y = run_data.rows;
  }

  if (run_data.cpos.y < run_data.row_bound.x)
    run_data.cpos.y = run_data.row_bound.x;
  if (run_data.cpos.x < run_data.col_bound.x)
    run_data.cpos.x = run_data.col_bound.x;

  draw_lines(lines_b, &left, &row);
  merge_lines(&ab, lines_b, row);
  char posbuf[32];
  snprintf(posbuf, sizeof(posbuf), "\x1b[%d;%dH", run_data.cpos.y + 1,
           run_data.cpos.x + 1);
  cpstr_append(&ab, posbuf, strlen(posbuf));
  cpstr_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.str, ab.len);
  cstr_free(&ab);
  for (int i = 0; i < run_data.rows; i++) {
    cstr_free(&(lines_b[i]));
  }
  free(lines_b);
}
void draw_settings() { return; };
void refresh() {
  if (run_data.mode == settings)
    draw_settings();
  else
    draw_viewer();
}
void read_lines(cstring filename, struct tab *out) {
  char *path = malloc(filename.len + 1);
  memcpy(path, filename.str, filename.len);
  path[filename.len] = '\0';
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  free(path);
  out->lines = (cstring_da){0};
  if (fd < 0) {
    cstring empty = CSTRING_INIT;
    csta_append(&out->lines, &empty);
    return;
  }
  char c;
  cstring cur_line = CSTRING_INIT;
  while (read(fd, &c, 1) == 1) {
    if (c == '\n') {
      csta_append(&out->lines, &cur_line);
      cur_line = (cstring)CSTRING_INIT;
      continue;
    }
    cchstr_append(&cur_line, c);
  }
  csta_append(&out->lines, &cur_line);
  close(fd);
}
int opentab(char *filename) {
  run_data.tabs_n += 1;
  struct tab *new_tabs =
      realloc(run_data.tabs, sizeof(struct tab) * run_data.tabs_n);
  if (!new_tabs) {
    run_data.tabs_n--;
    return -1;
  }
  run_data.tabs = new_tabs;
  struct tab *c_tab = &(run_data.tabs[run_data.tabs_n - 1]);
  memset(c_tab, 0, sizeof(struct tab));
  (c_tab->filename) = (cstring)CSTRING_INIT;
  cpstr_append(&(c_tab->filename), filename, strlen(filename));
  read_lines(c_tab->filename, c_tab);
  c_tab->line_scroll = calloc(c_tab->lines.n, sizeof(int));
  c_tab->mem_pos = (int2){0, 0};
  c_tab->selected = (cstring)CSTRING_INIT;
  return 0;
}
void initconf() {
  gwinsize(&(run_data.rows), &(run_data.cols));
  run_data.mode = edit;
  run_data.col_bound = (int2){3, run_data.cols};
  run_data.row_bound = (int2){0, run_data.rows};
  run_data.tabs_n = 0;
  run_data.staticconf.line_char = '*';
  run_data.staticconf.wordwrap = 0;
  run_data.staticconf.tab_style = 1;
}
int main(int argc, char **argv) {
  tcgetattr(STDIN_FILENO, &(run_data.orig_termios));
  cprint("\e[1;1H\e[2J");
  enableRawMode();
  initconf();
  for (int i = 1; i < argc; i++) {
    opentab(argv[i]);
  }
  if (run_data.tabs_n > 0) {
    refresh();
    run_data.cpos = (int2){run_data.col_bound.x, run_data.row_bound.x};
  }
  while (1) {
    gwinsize(&(run_data.rows), &(run_data.cols));
    refresh();
    process_input();
  }
  return 0;
}
