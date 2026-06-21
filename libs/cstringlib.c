#include "../btools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
void cstr_insert(cstring *insert, cstring *in, int idx) {
  if (idx < 0)
    idx = 0;
  if (idx > (int)in->len)
    idx = in->len;
  int new_len = in->len + insert->len;
  char *new_str = realloc(in->str, new_len);
  if (!new_str)
    return;
  in->str = new_str;
  memmove(in->str + idx + insert->len, in->str + idx, in->len - idx);
  memcpy(in->str + idx, insert->str, insert->len);
  in->len = new_len;
}
cstring cstr_cut(cstring *str, int start, int end) {
  cstring cstring_ret = CSTRING_INIT;
  if (start > end) {
    int x = start;
    start = end;
    end = x;
  }
  if (start < 0)
    start = 0;
  if (end > (int)str->len)
    end = str->len;
  if (end < start)
    end = start;
  cstring_ret.len = end - start;
  if (cstring_ret.len > 0) {
    cstring_ret.str = malloc(cstring_ret.len + 1);
    memcpy(cstring_ret.str, str->str + start, cstring_ret.len);
    cstring_ret.str[cstring_ret.len] = '\0';

    memmove(str->str + start, str->str + end, str->len - end);
    str->len -= cstring_ret.len;
    char *newbuf = realloc(str->str, str->len);
    if (newbuf || str->len == 0)
      str->str = newbuf;
  }
  return cstring_ret;
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
void chcdelete(cstring *str, int idx) {
  memmove(str->str + idx, str->str + idx + 1, str->len - idx - 1);
  str->len--;
  char *newbuf = realloc(str->str, str->len);
  str->str = newbuf;
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
  if (fg == reset) {
    cprint("\033[0m");
    return;
  }
  char color_esc[16];
  int len = sprintf(color_esc, "\x1b[%im\x1b[%im", fg, bg + 10);
  cpstr_append(str, color_esc, len);
}
void cstcol_idx(cstring *str, ecolor fg, ecolor bg, int idx) {
  if (fg == reset) {
    cprint("\033[0m");
    return;
  }
  char color_esc[16];
  int len = sprintf(color_esc, "\x1b[%im\x1b[%im", fg, bg + 10);
  for (int i = 0; i < len; i++) {
    chcinsert(str, idx + i, color_esc[i]);
  }
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
void csprint(cstring *str) { write(STDOUT_FILENO, str->str, str->len); }
int cstrvislen(cstring *str) {
  int vlen = 0;
  for (int i = 0; i < str->len; i++) {
    if (str->str[i] == '\x1b') {
      i++;
      if (i < str->len && str->str[i] == '[') {
        i++;
        while (i < str->len && str->str[i] != 'm')
          i++;
      }
    } else if (str->str[i] == '\t') {
      vlen += 4;
    } else {
      vlen++;
    }
  }
  return vlen;
}
void csta_append(cstring_da *array, cstring *str) {
  array->n++;
  array->strs = realloc(array->strs, sizeof(cstring) * array->n);
  array->strs[array->n - 1] = *str;
}
void csta_pop(cstring_da *array, int pop_idx) {
  memmove(array->strs + pop_idx, array->strs + pop_idx + 1,
          sizeof(cstring) * (array->n - pop_idx - 1));
  array->n--;
  array->strs = realloc(array->strs, sizeof(cstring) * array->n);
}
void csta_insert(cstring_da *array, cstring *str, int idx) {
  array->n++;
  array->strs = realloc(array->strs, sizeof(cstring) * array->n);
  memmove(array->strs + idx + 1, array->strs + idx,
          sizeof(cstring) * (array->n - idx - 1));
  array->strs[idx] = *str;
}
