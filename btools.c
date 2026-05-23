#include "btools.h"
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

void ccstr_append(cstring *main, cstring *app) {
  char *new = realloc(main->str, main->len + app->len);
  memcpy(&new[main->len], app->str, app->len);
  main->str = new;
  main->len += app->len;
}
void cpstr_append(cstring *main, char *app, int len) {
  char *new = realloc(main->str, main->len + len);
  memcpy(&new[main->len], app, len);
  main->str = new;
  main->len += len;
}
void cstr_free(cstring *str) {
  free(str->str);
  str->str = NULL;
  str->len = 0;
}
void cstr_replace(int a, int b, cstring *rep, cstring *with) {
  while ((int)rep->len < a) {
    cchstr_append(rep, ' ');
  }
  int removed = b - a + 1;
  int shift = (with ? with->len : 0) - removed;
  int new_len = rep->len + shift;
  char *new_str = realloc(rep->str, new_len);
  if (!new_str)
    return;
  if (b + 1 < (int)rep->len) {
    memmove(new_str + a + (with ? with->len : 0), new_str + b + 1,
            rep->len - b - 1);
  }
  if (with)
    memcpy(new_str + a, with->str, with->len);
  rep->len = new_len;
  rep->str = new_str;
}
void cchstr_append(cstring *main, char app) {
  char *new = realloc(main->str, main->len + 1);
  new[main->len] = app;
  main->len++;
  main->str = new;
}
void int_to_cstr(int n, cstring *str) {
  if (n == 0) {
    cchstr_append(str, '0');
    return;
  }
  if (n < 0) {
    cchstr_append(str, '-');
    n = -n;
  }
  int start = str->len;
  while (n > 0) {
    cchstr_append(str, n % 10 + '0');
    n /= 10;
  }
  for (int j = start, k = str->len - 1; j < k; j++, k--) {
    char temp = str->str[j];
    str->str[j] = str->str[k];
    str->str[k] = temp;
  }
}
int intlen(int n) {
  if (n == 0)
    return 1;
  if (n < 0)
    n = -n;
  int len = 0;
  while (n > 0) {
    len++;
    n /= 10;
  }
  return len;
}
void cstcol(cstring *str, ecolor fg, ecolor bg) {
  char color_esc[16];
  int len = sprintf(color_esc, "\x1b[%i;%im", fg, bg + 10);
  cpstr_append(str, color_esc, len);
}
int getrange(cstring *str, int range, int start, cstring *ret) {
  if (str->len < start + range)
    return 0;
  char *new = calloc(1, range);
  memcpy(new, str->str + start, range);
  *ret = (cstring){new, range};
  return 1;
}
void chcinsert(cstring *str, int idx, char ch) {
  char *newbuf = realloc(str->str, str->len + 1);
  str->str = newbuf;
  memmove(str->str + idx + 1, str->str + idx, str->len - idx);
  str->str[idx] = ch;
  str->len++;
}
