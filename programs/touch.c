#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include "../btools.h"
int touch(char *filename) {
    int fd=open(filename, O_WRONLY | O_CREAT,0644);
    if (fd==-1){
        cprint("shi.... unable to open it.\n");
    }
    close(fd);
    if (utime(filename, NULL) == -1) {
        cprint("shi... unable to do time shit\n");
        return -1;
    }
    return 0;
}
int main(int argc,char **argv){
    for (int i=1;i<argc;i++){
        if (touch(argv[i])!=0){
            cprint("shi.. unable to open da file\n");
        }
    }
}