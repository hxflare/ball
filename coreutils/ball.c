#include "../btools.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enable_term_rawmode() {
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

typedef enum exec_types {
  normal = 0,
  piped_out_of = 1,
  overwrite_file = 2,
  append_to_file = 3,
  eor = 4,
  background = 5,
  and = 6
} exec_types;

typedef struct exec_batch {
  char *command;
  exec_types type;
} exec_batch;

typedef struct shellConf {
  char aliases[255][255];
  char meanings[255][255];
  char PISS[255];
  char paths[255][255];
} shellConf;

char **extract_args(char *command, shellConf config, int *argc) {
  int raw_c = 1;
  int command_len = strlen(command);
  for (int i = 1; i < command_len; i++) {
    if (command[i] == ' ' && command[i - 1] != ' ') {
      raw_c++;
    }
  }
  char **args = malloc((raw_c + 1) * sizeof(char *));
  if (!args)
    return NULL;
  {
    int arg_index = 0;
    int i = 0;
    while (i < command_len) {
      while (i < command_len && command[i] == ' ')
        i++;
      if (i >= command_len)
        break;
      char cur_arg[256];
      int cur_len = 0;
      if (command[i] == '"') {
        i++;
        while (i < command_len && command[i] != '"') {
          cur_arg[cur_len++] = command[i++];
        }
        if (i < command_len)
          i++;
      } else {
        while (i < command_len && command[i] != ' ') {
          if (command[i] == '~') {
            i++;
            char *username = getenv("HOME");
            if (username) {
              for (int p = 0; p < (int)strlen(username) && cur_len < 255; p++) {
                cur_arg[cur_len++] = username[p];
              }
            }
          } else {
            if (cur_len < 255)
              cur_arg[cur_len++] = command[i++];
            else
              i++;
          }
        }
      }
      cur_arg[cur_len] = '\0';
      args[arg_index++] = strdup(cur_arg);
    }
    args[arg_index] = NULL;
  }
  char **aliased = malloc((raw_c * 16 + 2) * sizeof(char *));
  if (!aliased)
    return NULL;
  {
    int arg_index = 0;
    int i = 0;
    while (args[i] != NULL) {
      int j = 0;
      while (config.aliases[j][0] != '\0') {
        if (strcmp(config.aliases[j], args[i]) == 0) {
          char *replaced =
              str_replace(args[i], config.aliases[j], config.meanings[j]);
          if (replaced != NULL) {
            free(args[i]);
            args[i] = replaced;
          }
        }
        j++;
      }
      char *cur = args[i];
      int cur_len_total = strlen(cur);
      i++;
      int k = 0;
      while (k < cur_len_total) {
        char cur_arg[256];
        int cur_len = 0;
        while (k < cur_len_total && cur[k] == ' ')
          k++;
        if (k >= cur_len_total)
          break;
        if (cur[k] == '"') {
          k++;
          while (k < cur_len_total && cur[k] != '"') {
            if (cur_len < 255)
              cur_arg[cur_len++] = cur[k];
            k++;
          }
          if (k < cur_len_total)
            k++;
        } else {
          while (k < cur_len_total && cur[k] != ' ') {
            if (cur[k] == '~') {
              k++;
              char *homedir = getenv("HOME");
              if (homedir) {
                for (int p = 0; p < (int)strlen(homedir) && cur_len < 255;
                     p++) {
                  cur_arg[cur_len++] = homedir[p];
                }
              }
            } else {
              if (cur_len < 255)
                cur_arg[cur_len++] = cur[k++];
              else
                k++;
            }
          }
        }
        cur_arg[cur_len] = '\0';
        aliased[arg_index++] = strdup(cur_arg);
      }
    }
    *argc = arg_index;
    aliased[arg_index] = NULL;
    free(args);
    return aliased;
  }
}

exec_batch *get_exec_order(char *raw) {
  int len = strlen(raw);
  char *cur_command = malloc(256);
  if (!cur_command)
    return NULL;
  int cur_len = 0;
  int del_amount = 0;
  for (int i = 0; i < len; i++) {
    if (raw[i] == ';' || raw[i] == '|' || raw[i] == '&' || raw[i] == '>' ||
        raw[i] == '^' || raw[i] == '@' || raw[i] == ':') {
      del_amount++;
    }
  }
  exec_batch *order = malloc(sizeof(exec_batch) * (del_amount + 2));
  if (!order)
    return NULL;
  memset(order, 0, sizeof(exec_batch) * (del_amount + 2));
  int i = 0;
  int quoted = 0;
  int index = 0;
  while (raw[i] != '\0') {
    if (raw[i] == '"') {
      quoted = !quoted;
    }
    if ((raw[i] == ';' || raw[i] == '|' || raw[i] == '&' || raw[i] == '>' ||
         raw[i] == '^' || raw[i] == '@' || raw[i] == ':') &&
        quoted == 0) {
      if (cur_len > 0) {
        cur_command[cur_len] = '\0';
        cur_len = 0;
        order[index].command = strdup(cur_command);
        switch (raw[i]) {
        case ';':
          order[index].type = normal;
          break;
        case '|':
          order[index].type = piped_out_of;
          break;
        case '^':
          order[index].type = overwrite_file;
          break;
        case '>':
          order[index].type = append_to_file;
          break;
        case '&':
          order[index].type = background;
          break;
        case '@':
          order[index].type = and;
          break;
        case ':':
          order[index].type = eor;
          break;
        default:
          order[index].type = normal;
          break;
        }
        i++;
        index++;
      }
    } else {
      if (cur_len < 255) {
        cur_command[cur_len] = raw[i];
        cur_len++;
      }
    }
    i++;
  }
  if (cur_len > 0) {
    cur_command[cur_len] = '\0';
    order[index].command = strdup(cur_command);
    order[index].type = normal;
    index++;
  }
  order[index].command = NULL;
  free(cur_command);
  return order;
}

char *formatPISS(shellConf config) {
  int normal_len = strlen(config.PISS);
  char *prompt = malloc(1024);
  if (!prompt)
    return NULL;
  char *PISS = config.PISS;
  int cur_char_i = 0;
  for (int i = 0; i < normal_len; i++) {
    if (PISS[i] != '%') {
      if (cur_char_i < 1023)
        prompt[cur_char_i++] = PISS[i];
    } else {
      char escape_ident = PISS[i + 1];
      i += 1;
      char *insert_str = "";
      char cwd_buf[256];
      char host_buf[256];
      switch (escape_ident) {
      case 'n':
        insert_str = "\n";
        break;
      case 'u':
        insert_str = getenv("USER");
        if (!insert_str)
          insert_str = "root";
        break;
      case 'p': {
        char *homedir = getenv("HOME");
        char *cwd = getcwd(cwd_buf, sizeof(cwd_buf));
        if (!cwd)
          cwd = "/";
        if (homedir) {
          char *replaced = str_replace(cwd, homedir, "~");
          insert_str = replaced ? replaced : cwd;
        } else {
          insert_str = cwd;
        }
        break;
      }
      case 'P':
        insert_str = getcwd(cwd_buf, sizeof(cwd_buf));
        if (!insert_str)
          insert_str = "/";
        break;
      case 'h':
        if (gethostname(host_buf, sizeof(host_buf)) == 0)
          insert_str = host_buf;
        else
          insert_str = "localhost";
        break;
      default:
        insert_str = "";
        break;
      }
      int insert_len = strlen(insert_str);
      for (int k = 0; k < insert_len && cur_char_i < 1023; k++) {
        prompt[cur_char_i++] = insert_str[k];
      }
    }
  }
  prompt[cur_char_i] = '\0';
  return prompt;
}

int execute(char mode, char *execd, shellConf config) {
  switch (mode) {
  case 'c': {
    extern char **environ;
    exec_batch *order = get_exec_order(execd);
    if (!order)
      return EXIT_FAILURE;
    int i = 0;
    while (order[i].command != NULL) {
      int argc = 0;
      char **args = extract_args(order[i].command, config, &argc);
      if (!args || !args[0] || args[0][0] == '\0') {
        i++;
        continue;
      }
      if (strcmp(args[0], "cd") == 0) {
        const char *dir = (argc > 1) ? args[1] : getenv("HOME");
        if (!dir)
          dir = "/";
        if (chdir(dir) != 0)
          perror("cd");
        i++;
        continue;
      } else if (strcmp(args[0], "clear") == 0) {
        cprint("\e[1;1H\e[2J");
        i++;
        continue;
      } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
      } else if (strcmp(args[0], "export") == 0 && argc > 1) {
        char *eq = strchr(args[1], '=');
        if (eq) {
          *eq = '\0';
          setenv(args[1], eq + 1, 1);
        }
        i++;
        continue;
      }
      pid_t forked = fork();
      int status;
      if (forked == 0) {
        disableRawMode();
        if (strchr(args[0], '/')) {
          execve(args[0], args, environ);
          exit(1);
        } else {
          int k = 0;
          while (config.paths[k][0] != '\0') {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", config.paths[k],
                     args[0]);
            execve(full_path, args, environ);
            k++;
          }
          exit(127);
        }
      } else if (forked == -1) {
        cprint("fork failed\n");
      } else {
        if (waitpid(forked, &status, 0) == -1) {
          cprint("waitpid error\n");
        }
        disableRawMode();
        enable_term_rawmode();
      }
      i++;
    }
    break;
  }
  case 'f':
    break;
  default:
    cprint("invalid mode\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

shellConf getConf(FILE *rc, int execute_commands) {
  char line[1024];
  char loaded[90][1024];
  int ammount = 0;
  shellConf config;
  memset(&config, 0, sizeof(shellConf));
  while (fgets(line, sizeof(line), rc) != NULL && ammount < 90) {
    strcpy(loaded[ammount], line);
    ammount++;
  }
  for (int i = 0; i < ammount; i++) {
    if (loaded[i][0] == '!') {
      char keyword[16];
      char values[50][255];
      int valueCount = 0;
      loaded[i][strcspn(loaded[i], "\n")] = 0;
      strncpy(keyword, loaded[i], sizeof(keyword) - 1);
      keyword[sizeof(keyword) - 1] = '\0';
      for (int j = i + 1; j < ammount && valueCount < 50; j++) {
        if (loaded[j][0] == '@') {
          strncpy(values[valueCount], loaded[j], 254);
          values[valueCount][254] = '\0';
          valueCount++;
        } else if (loaded[j][0] == '!') {
          break;
        }
      }
      if (strcmp(keyword, "!PISS") == 0) {
        if (valueCount > 0) {
          char *format = values[0] + 1;
          format[strcspn(format, "\n")] = 0;
          strncpy(config.PISS, format, 254);
        }
      } else if (strcmp(keyword, "!ALIAS") == 0) {
        for (int j = 0; j < valueCount; j++) {
          char *format = values[j] + 1;
          format[strcspn(format, "\n")] = 0;
          char *eq = strchr(format, '=');
          if (!eq)
            continue;
          *eq = '\0';
          strncpy(config.aliases[j], format, 254);
          strncpy(config.meanings[j], eq + 1, 254);
        }
      } else if (strcmp(keyword, "!PATH") == 0) {
        for (int j = 0; j < valueCount; j++) {
          char *format = values[j] + 1;
          format[strcspn(format, "\n")] = 0;
          strncpy(config.paths[j], format, 254);
        }
      }
    }
  }
  if (execute_commands) {
    for (int i = 0; i < ammount; i++) {
      if (loaded[i][0] == '$') {
        execute('c', loaded[i] + 1, config);
      }
    }
  }
  return config;
}

void loop(shellConf config) {
  enable_term_rawmode();
  char *piss = formatPISS(config);
  cprint(piss);
  free(piss);
  char *command = malloc(256);
  int buf_size = 256;
  if (!command)
    return;
  int index = 0;
  char cur_c;
  while (read(STDIN_FILENO, &cur_c, 1) == 1) {
    if (cur_c == '\n' || cur_c == '\r') {
      cprint("\n");
      command[index] = '\0';
      index = 0;
      if (command[0] != '\0')
        execute('c', command, config);
      piss = formatPISS(config);
      cprint("\n");
      cprint(piss);
      free(piss);
    } else if (cur_c == 127 || cur_c == '\b') {
      if (index > 0) {
        index--;
        cprint("\b \b");
      }
    } else if (cur_c == 3) {
      cprint("^C\n");
      index = 0;
      piss = formatPISS(config);
      cprint(piss);
      free(piss);
    } else if (cur_c == '\x1b') {
      char seq[4] = {0};
      read(STDIN_FILENO, &seq[0], 1);
      read(STDIN_FILENO, &seq[1], 1);
      kkey_t key;
      if (seq[0] == '[') {
        switch (seq[1]) {
        case 'A':
          key = arrow_up;
          break;
        case 'B':
          key = arrow_down;
          break;
        case 'C':
          key = arrow_right;
          break;
        case 'D':
          key = arrow_left;
          break;
        }
      }
    } else {
      if (index >= buf_size - 1) {
        buf_size *= 2;
        command = realloc(command, buf_size);
        if (!command)
          return;
      }
      command[index++] = cur_c;
      write(1, &cur_c, 1);
    }
  }
  free(command);
}

int main(int argc, char **argv) {
  tcgetattr(STDIN_FILENO, &orig_termios);
  FILE *rcfile;
  shellConf conf;
  memset(&conf, 0, sizeof(shellConf));

  char *rc_path = concat(getenv("HOME"), "/.ballrc");
  rcfile = fopen(rc_path, "r");

  if (rcfile == NULL) {
    rcfile = fopen(rc_path, "w");
    if (rcfile != NULL) {
      fprintf(rcfile,
              "The ball shell configuration file.\n"
              "!PISS\n@%%u@%%h  %%p %%n#   \n"
              "!ALIAS\n@ls=l -cthi\n"
              "!PATH\n@/bin\n@/usr/bin\n@/usr/local/bin\n@/sbin\n@/usr/sbin\n");
      fclose(rcfile);

      rcfile = fopen(rc_path, "r");
    }
  }
  free(rc_path);
  if (rcfile != NULL) {
    if (argc > 1) {
      conf = getConf(rcfile, 0);
    } else {
      conf = getConf(rcfile, 1);
    }
    fclose(rcfile);
  } else {
    strncpy(conf.PISS, "@%%u@%%h  %%p %%n#  ", 254);
    strncpy(conf.paths[0], "/bin", 254);
    strncpy(conf.paths[1], "/usr/bin", 254);
    strncpy(conf.paths[2], "/sbin", 254);
  }

  if (argc > 1) {
    if (argv[1][0] == '-' && argv[1][1] == 'c') {
      if (argc < 3)
        return 0;

      char *arg = strdup("");
      for (int i = 2; i < argc; i++) {
        char *with_arg = concat(arg, argv[i]);
        free(arg);
        char *with_space = concat(with_arg, " ");
        free(with_arg);
        arg = with_space;
      }

      execute('c', arg, conf);
      free(arg);
    } else {
      for (int i = 1; i < argc; i++) {
        execute('f', argv[i], conf);
      }
    }

  } else {
    loop(conf);
  }
  return 0;
}
