#ifndef BTOOLS_H
#define BTOOLS_H
#define CSTRING_INIT {NULL,0}
typedef enum {
  undefined = 1024,
  arrow_up = 1,
  arrow_right = 2,
  arrow_left = 3,
  arrow_down = 4
} kkey_t;
typedef struct cstring {
  char *str;
  int len;
} cstring;
void cstr_free(cstring *str);
void cpstr_append(cstring *main, char *app, int len);
void ccstr_append(cstring *main, cstring *app);
void cchstr_append(cstring *main, char app);
void gcpos(int *rows, int *cols);
void gwinsize(int *rows, int *cols);
void cprint(const char *string);
char *str_replace(char *orig, char *rep, char *with);
void print_strlist(const char **array);
char *concat(const char *s1, const char *s2);
int str_isdigit(const char *str);
void int_to_cstr(int n, cstring *str);
int intlen(int n);
#endif
