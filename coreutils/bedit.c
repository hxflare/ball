#include "../btools.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
// data structures
struct editorConfig {
  int rows;
  int cols;
  int crows;
  int ccols;
  struct termios orig_termios;
};
struct editorConfig config;

// functionality
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(config.orig_termios));
}
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &(config.orig_termios));
  atexit(disableRawMode);
  struct termios raw = config.orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON);
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
  if (config.crows > config.rows)
    config.crows = config.rows;
  if (config.ccols > config.cols)
    config.ccols = config.ccols;
  if (config.crows < 0)
    config.crows = 0;
  if (config.ccols < 0)
    config.ccols = 0;
}
void process() {
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return;
  switch (c) {
  case CTRL_KEY('x'):
    cprint("Exitins");
    exit(0);
  case 'w':
  case 's':
  case 'a':
  case 'd':
    cmove(c);
    break;
  }
}
void draw_tildes(cstring *ab) {
  for (int y = 0; y < config.rows; y++) {
    cpstr_append(ab, "~", 1);
    cpstr_append(ab, "\x1b[K", 3);
    if (y < config.rows - 1) {
      cpstr_append(ab, "\r\n", 2);
    }
  }
}
void refresh() {
  cstring ab = ABUF_INIT;
  cpstr_append(&ab, "\x1b[?25l", 6);
  cpstr_append(&ab, "\x1b[H", 3);
  draw_tildes(&ab);
  char posbuf[32];
  snprintf(posbuf, sizeof(posbuf), "\x1b[%d;%dH", config.crows + 1,
           config.ccols + 1);
  cpstr_append(&ab, posbuf, strlen(posbuf));
  cpstr_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.str, ab.len);
  cstr_free(&ab);
}
// init
void initconf() {
  gwinsize(&(config.rows), &(config.cols));
  config.crows = 0;
  config.ccols = 0;
}
int main() {
  enableRawMode();
  initconf();
  while (1) {
    refresh();
    process();
  }
  return 0;
}
