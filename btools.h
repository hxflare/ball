#ifndef BTOOLS_H
#define BTOOLS_H
#define CSTRING_INIT (cstring){NULL, 0}
#define CSTRING_DA_INIT (cstring_da){NULL, 0}
#define INT2_INIT (int2){0, 0}
typedef enum {
  KEY_NULL = 0,
  KEY_CTRL_C = 3,
  KEY_ENTER = 13,
  KEY_ESC = 27,
  KEY_BACKSPACE = 127,
  KEY_ARROW_UP = 1000,
  KEY_ARROW_DOWN,
  KEY_ARROW_LEFT,
  KEY_ARROW_RIGHT,
  KEY_CTRL_ARROW_LEFT,
  KEY_CTRL_ARROW_RIGHT,
  KEY_CTRL_ARROW_UP,
  KEY_CTRL_ARROW_DOWN,
  KEY_SHIFT_ARROW_UP,
  KEY_SHIFT_ARROW_DOWN,
  KEY_SHIFT_ARROW_LEFT,
  KEY_SHIFT_ARROW_RIGHT,
  KEY_CTRL_SHIFT_ARROW_UP,
  KEY_CTRL_SHIFT_ARROW_DOWN,
  KEY_CTRL_SHIFT_ARROW_LEFT,
  KEY_CTRL_SHIFT_ARROW_RIGHT,
  KEY_SHIFT_TAB,
} kkey_t;
typedef enum {
  white = 37,
  black = 30,
  red = 31,
  green = 32,
  yellow = 33,
  blue = 34,
  magenta = 35,
  cyan = 36,
  reset = 0,
} ecolor;
typedef struct cstring {
  char *str;
  int len;
} cstring;
typedef struct cstring_da {
  cstring *strs;
  int n;
} cstring_da;
typedef struct int2 {
  int x, y;
} int2;
void cstr_free(cstring *str);
void cpstr_append(cstring *main, char *app, int len);
void ccstr_append(cstring *main, cstring *app);
void cchstr_append(cstring *main, char app);
void cstr_insert(cstring *insert, cstring *in, int idx);
cstring cstr_cut(cstring *str, int start, int end);
void gcpos(int *rows, int *cols);
void cr_move(int row, int col);
void cr_rel_move(int row, int col);
void gwinsize(int *rows, int *cols);
void cprint(const char *string);
char first_none_space(char *str);
int ends_with(char *str, char *suffix);
int starts_with(char *str, char *prefix);
int read_key();
char *str_replace(char *orig, char *rep, char *with);
void print_strlist(const char **array);
char *concat(const char *s1, const char *s2);
int str_isdigit(const char *str);
void int_to_cstr(int n, cstring *str);
int intlen(int n);
int getrange(cstring *str, int range, int start, cstring *ret);
void cstr_replace(int a, int b, cstring *rep, cstring *with);
void cstcol(cstring *str, ecolor fg, ecolor bg);
void cstcol_idx(cstring *str, ecolor fg, ecolor bg, int idx);
void setcol(ecolor fg, ecolor bg);
void chcinsert(cstring *str, int idx, char ch);
void chcdelete(cstring *str, int idx);
void csprint(cstring *str);
int cstrvislen(cstring *str);
void csta_pop(cstring_da *array, int pop_idx);
void csta_append(cstring_da *array, cstring *str);
void csta_insert(cstring_da *array, cstring *str, int idx);
#endif
