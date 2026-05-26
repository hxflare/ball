#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    return 1;
  }
  int src = open(argv[1], O_RDONLY);
  if (src == -1) {
    write(2, "open failed\n", 12);
    return 1;
  }
  struct stat st;
  fstat(src, &st);
  int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
  if (dst == -1) {
    write(2, "open dest failed\n", 18);
    close(src);
    return 1;
  }

  char buf[4096];
  ssize_t n;
  while ((n = read(src, buf, sizeof(buf))) > 0) {
    write(dst, buf, n);
  }
  close(src);
  close(dst);
  return 0;
}