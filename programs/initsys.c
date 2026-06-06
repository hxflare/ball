#include "../btools.h"
#include <bits/pthreadtypes.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
  char name[64];
  char after[16][64];
  int after_count;
  char before[16][64];
  int before_count;
  char functional[256][256];
  char type[32];
} Service;

typedef struct {
  int indegree;
  int visited;
  int after_count;
  int after[16];
  int before_count;
  int before[16];
  char *name;
} Node;

Service *get_service_by_name(char *name, Service *service_array) {
  int i = 0;
  while (service_array[i].name[0] != '\0') {
    if (strcmp(name, service_array[i].name) == 0) {
      return &service_array[i];
    }
    i++;
  }
  return NULL;
}

int get_service_index_by_name(char *name, Service *service_array) {
  int i = 0;
  while (service_array[i].name[0] != '\0') {
    if (strcmp(name, service_array[i].name) == 0) {
      return i;
    }
    i++;
  }
  return -1;
}

Node *get_node_by_name(char *name, Node *nodes, int amount) {
  for (int i = 0; i < amount; i++) {
    if (strcmp(name, nodes[i].name) == 0) {
      return &nodes[i];
    }
  }
  return NULL;
}

Node *nodeize(Service *services, int amount) {
  Node *nodes = malloc(sizeof(Node) * amount);
  for (int i = 0; i < amount; i++) {
    nodes[i].name = strdup(services[i].name);
    nodes[i].indegree = 0;
    nodes[i].before_count = 0;
    nodes[i].after_count = 0;
    nodes[i].visited = 0;
  }
  for (int i = 0; i < amount; i++) {
    Node *cur_node = &nodes[i];
    Service *cur_service = &services[i];
    for (int k = 0; k < cur_service->before_count; k++) {
      int idx = get_service_index_by_name(cur_service->before[k], services);
      if (idx < 0)
        continue;
      Node *bind_node = &nodes[idx];
      int already = 0;
      for (int j = 0; j < bind_node->before_count; j++) {
        if (bind_node->before[j] == i) {
          already = 1;
          break;
        }
      }
      if (already)
        continue;
      bind_node->before[bind_node->before_count++] = i;
      int already2 = 0;
      for (int j = 0; j < cur_node->after_count; j++) {
        if (cur_node->after[j] == idx) {
          already2 = 1;
          break;
        }
      }
      if (!already2)
        cur_node->after[cur_node->after_count++] = idx;
    }
    for (int k = 0; k < cur_service->after_count; k++) {
      int idx = get_service_index_by_name(cur_service->after[k], services);
      if (idx < 0)
        continue;
      int already = 0;
      for (int j = 0; j < cur_node->before_count; j++) {
        if (cur_node->before[j] == idx) {
          already = 1;
          break;
        }
      }
      if (already)
        continue;
      cur_node->before[cur_node->before_count++] = idx;
      Node *bind_node = &nodes[idx];
      int already2 = 0;
      for (int j = 0; j < bind_node->after_count; j++) {
        if (bind_node->after[j] == i) {
          already2 = 1;
          break;
        }
      }
      if (!already2)
        bind_node->after[bind_node->after_count++] = i;
    }
  }
  return nodes;
}

int *topological_order(Service *services, int amount) {
  Node *nodes = nodeize(services, amount);
  int *order = malloc(sizeof(int) * amount);
  int *queue = malloc(sizeof(int) * amount);
  int qstart = 0;
  int qend = 0;
  for (int i = 0; i < amount; i++) {
    nodes[i].indegree = nodes[i].before_count;
    if (nodes[i].indegree == 0) {
      queue[qend++] = i;
    }
  }
  int index = 0;
  while (qstart < qend) {
    int cur = queue[qstart++];
    order[index++] = cur;
    for (int k = 0; k < nodes[cur].after_count; k++) {
      int next = nodes[cur].after[k];
      nodes[next].indegree--;
      if (nodes[next].indegree == 0) {
        queue[qend++] = next;
      }
    }
  }
  if (index != amount) {
    cprint("shi... dependecy cycle detected\n");
  }
  for (int i = 0; i < amount; i++)
    free(nodes[i].name);
  free(nodes);
  free(queue);
  return order;
}

Service parse_service(char *service_name) {
  Service service;
  memset(&service, 0, sizeof(Service));
  strncpy(service.name, service_name, 64);
  char *path = concat("/etc/initsys.d/", service_name);
  FILE *rc = fopen(path, "r");
  free(path);
  if (rc == NULL) {
    cprint("Failed to open service file\n");
    return service;
  }
  char fline[1024];
  char (*lines)[1024] = malloc(1024 * 1024);
  int amm = 0;
  while (fgets(fline, sizeof(fline), rc) != NULL) {
    strncpy(lines[amm], fline, 1024);
    lines[amm][strcspn(lines[amm], "\n")] = 0;
    amm++;
  }
  fclose(rc);
  int cur_line = 0;
  while (cur_line < amm) {
    if (lines[cur_line][0] == '!') {
      if (strcmp(lines[cur_line] + 1, "after") == 0) {
        int count = 0;
        cur_line++;
        while (cur_line < amm && lines[cur_line][0] != '!') {
          if (lines[cur_line][0] == '@') {
            strncpy(service.after[count], lines[cur_line] + 1, 64);
            count++;
          }
          cur_line++;
        }
        service.after_count = count;
      } else if (strcmp(lines[cur_line] + 1, "before") == 0) {
        int count = 0;
        cur_line++;
        while (cur_line < amm && lines[cur_line][0] != '!') {
          if (lines[cur_line][0] == '@') {
            strncpy(service.before[count], lines[cur_line] + 1, 64);
            count++;
          }
          cur_line++;
        }
        service.before_count = count;
      } else if (strcmp(lines[cur_line] + 1, "type") == 0) {
        cur_line++;
        while (cur_line < amm && lines[cur_line][0] != '!') {
          if (lines[cur_line][0] == '@') {
            strncpy(service.type, lines[cur_line] + 1, 32);
            cur_line++;
            break;
          }
          cur_line++;
        }
      } else if (strcmp(lines[cur_line] + 1, "functional") == 0) {
        cur_line++;
        int count = 0;
        while (cur_line < amm && lines[cur_line][0] != '!') {
          strncpy(service.functional[count], lines[cur_line], 256);
          count++;
          cur_line++;
        }
        service.functional[count][0] = '\0';
      } else {
        cur_line++;
      }
    } else {
      cur_line++;
    }
  }
  free(lines);
  return service;
}

void print_service(Service service) {
  cprint("name: ");
  cprint(service.name);
  cprint("\n");
  cprint("type: ");
  cprint(service.type);
  cprint("\n");
  cprint("functional: ");
  int i = 0;
  while (service.functional[i][0] != '\0') {
    cprint(service.functional[i]);
    cprint("\n");
    i++;
  }
  cprint("\n");
  cprint("after: ");
  for (int i = 0; i < service.after_count; i++) {
    cprint(service.after[i]);
    if (i < service.after_count - 1)
      cprint(", ");
  }
  cprint("\n");
  cprint("before: ");
  for (int i = 0; i < service.before_count; i++) {
    cprint(service.before[i]);
    if (i < service.before_count - 1) {
      cprint(", ");
    }
  }

  cprint("\n");
}

void execute_service(Service *service) {
  if (strcmp(service->type, "execute") == 0) {
    pid_t fork_pid;
    extern char **environ;
    int i = 0;
    while (service->functional[i][0] != '\0') {
      fork_pid = fork();

      if (fork_pid == 0) {
        char *args[] = {service->functional[i], NULL};
        execve(service->functional[i], args, environ);
        perror("execve failed");
        exit(EXIT_FAILURE);
      } else if (fork_pid == -1) {
        cprint("fork failed\n");
      } else {
        int status;
        waitpid(fork_pid, &status, 0);
      }
      i++;
    }
  } else if (strcmp(service->type, "write") == 0) {
    int i = 0;
    while (service->functional[i][0] != '\0') {
      cprint(service->functional[i]);
      i++;
    }
  } else if (strcmp(service->type, "command") == 0) {
    pid_t fork_pid;
    extern char **environ;
    int i = 0;
    while (service->functional[i][0] != '\0') {
      fork_pid = fork();

      if (fork_pid == 0) {
        char *inner = concat("\"", service->functional[i]);
        char *func = concat(inner, "\"");
        free(inner);
        char *args[] = {"/bin/ball", "-c", func, NULL};
        execve("/bin/ball", args, environ);
        perror("execve failed");
        exit(EXIT_FAILURE);
      } else if (fork_pid == -1) {
        cprint("fork failed\n");
      } else {
        int status;
        waitpid(fork_pid, &status, 0);
      }
      i++;
    }
  } else if (strcmp(service->type, "daemon") == 0) {
    pid_t fork_pid;
    extern char **environ;
    int i = 0;
    while (service->functional[i][0] != '\0') {
      fork_pid = fork();

      if (fork_pid == 0) {
        char *args[] = {service->functional[i], NULL};
        execve(service->functional[i], args, environ);
        perror("execve failed");
        exit(EXIT_FAILURE);
      } else if (fork_pid == -1) {
        cprint("fork failed\n");
      }
      i++;
    }
  }
}

int main(int argc, char **argv) {
  cprint("initsys starting. getting services\n");
  Service *service_root;
  DIR *initsys_dir;
  struct dirent *entry;
  char *path = "/etc/initsys.d";
  initsys_dir = opendir(path);
  if (initsys_dir == NULL) {
    cprint("Unable to open /etc/initsys.d\n");
    exit(EXIT_FAILURE);
  }
  int amount = 0;
  service_root = malloc(sizeof(Service) * (amount + 1));
  while ((entry = readdir(initsys_dir)) != NULL) {
    char *name = entry->d_name;
    unsigned char type = entry->d_type;
    if (name[0] != '.') {
      if (type == DT_REG) {
        service_root[amount] = parse_service(name);
        amount++;
        service_root = realloc(service_root, sizeof(Service) * (amount + 1));
      } else {
        cprint("Only regular files are parsed as services. ");
        cprint(name);
        cprint(" is not a service.\n");
      }
    }
  }
  service_root[amount].name[0] = '\0';
  int *order = topological_order(service_root, amount);
  for (int i = 0; i < amount; i++) {
    execute_service(&service_root[order[i]]);
  }
  while (1)
    wait(NULL);
}
