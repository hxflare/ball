#include <unistd.h>
#include "../btools.h"
size_t strlen(const char* start)
{
    const char* end = start;
    while (*end != '\0')
        ++end;
    return end - start;
}
int main(int argc, char **argv){
    cprint("\e[1;1H\e[2J");
}