#ifndef BTOOLS_H
#define BTOOLS_H
typedef enum  { undefined=1024,arrow_up = 1,arrow_right=2, arrow_left=3,arrow_down=4}key_t;
void cprint(const char *string);
char *str_replace(char *orig, char *rep, char *with);
void print_strlist(const char **array);
char *concat(const char *s1, const char *s2);
int str_isdigit(const char *str);
#endif
