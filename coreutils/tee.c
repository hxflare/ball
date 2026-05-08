#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../btools.h"
int main(int argc,char **argv){
    int quoted=0;
    if (argc<3){
        return 1;
    }
    int fd=open(argv[2],O_WRONLY | O_CREAT | O_TRUNC,0644);
    if(fd==-1){
        cprint("file aint opening\n");
        return 1;
    }
    write(fd, argv[1], strlen(argv[1]));

}
