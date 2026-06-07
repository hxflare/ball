#include "../btools.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void cprint(const char *string) {
  if (!string)
    return;
  write(1, string, strlen(string));
}
void gcpos(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  write(STDOUT_FILENO, "\x1b[6n", 4);
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';
  sscanf(&buf[2], "%d;%d", rows, cols);
}
void cr_move(int row, int col) {
  char posbuf[32];
  snprintf(posbuf, sizeof(posbuf), "\x1b[%d;%dH", row, col);
  write(STDOUT_FILENO, posbuf, strlen(posbuf));
}
void cr_rel_move(int row, int col) {
  int crow;
  int ccol;
  gcpos(&crow, &ccol);
  cr_move(crow + row, ccol + col);
}
void gwinsize(int *rows, int *cols) {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  *cols = ws.ws_col;
  *rows = ws.ws_row;
}

char *str_replace(char *orig, char *rep, char *with) {
  char *result;
  char *ins;
  char *tmp;
  int len_rep;
  int len_with;
  int len_front;
  int count;
  if (!orig || !rep)
    return NULL;
  len_rep = strlen(rep);
  if (len_rep == 0)
    return NULL;
  if (!with)
    with = "";
  len_with = strlen(with);
  ins = orig;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }
  tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
  if (!result)
    return NULL;
  while (count--) {
    ins = strstr(orig, rep);
    len_front = ins - orig;
    tmp = strncpy(tmp, orig, len_front) + len_front;
    tmp = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

char *concat(const char *s1, const char *s2) {
  if (!s1)
    s1 = "";
  if (!s2)
    s2 = "";
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  if (!result)
    return NULL;
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

void print_strlist(const char **array) {
  if (!array)
    return;
  for (int i = 0; array[i] != NULL; i++) {
    cprint(array[i]);
  }
}

int str_isdigit(const char *str) {
  if (!str || strlen(str) == 0)
    return 0;
  for (int i = 0; i < (int)strlen(str); i++) {
    if (isdigit((unsigned char)str[i]) == 0) {
      return 0;
    }
  }
  return 1;
}
void setcol(ecolor fg, ecolor bg) {
  if (fg == reset) {
    cprint("\033[0m");
    return;
  }
  char color_esc[16];
  int len = sprintf(color_esc, "\x1b[%i;%im", fg, bg + 10);
  write(1, color_esc, len);
}
int read_key() {
  char c;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return KEY_NULL;
  if (c != '\x1b')
    return c;

  char seq[8];
  int seq_len = 0;

  while (seq_len < (int)sizeof(seq) - 1) {
    if (read(STDIN_FILENO, &seq[seq_len], 1) != 1)
      break;
    seq_len++;
    char last = seq[seq_len - 1];
    if ((last >= 'A' && last <= 'Z') || (last >= 'a' && last <= 'z') ||
        last == '~')
      break;
  }
  seq[seq_len] = '\0';

  if (seq_len == 0)
    return KEY_ESC;

  if (seq[0] == '[') {
    if (seq_len >= 5 && seq[1] == '1' && seq[2] == ';') {
      int modifier = seq[3] - '0';
      if (modifier == 5) {
        switch (seq[4]) {
        case 'A':
          return KEY_CTRL_ARROW_UP;
        case 'B':
          return KEY_CTRL_ARROW_DOWN;
        case 'C':
          return KEY_CTRL_ARROW_RIGHT;
        case 'D':
          return KEY_CTRL_ARROW_LEFT;
        }
      }
    }
    switch (seq[1]) {
    case 'A':
      return KEY_ARROW_UP;
    case 'B':
      return KEY_ARROW_DOWN;
    case 'C':
      return KEY_ARROW_RIGHT;
    case 'D':
      return KEY_ARROW_LEFT;
    }
  }
  return KEY_ESC;
}
char first_none_space(char *str) {
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] != ' ') {
      return str[i];
    }
  }
  return '\0';
}
