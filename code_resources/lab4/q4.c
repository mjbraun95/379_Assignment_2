#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main()
{

    int len;
    char buf[80];

    // while(1) {
    //     len = read(STDIN_FILENO, buf, sizeof(buf));
    //     write(STDOUT_FILENO, buf, sizeof(buf));
    //     if (strstr(buf, "exit") != NULL) exit(0);
    // }

    while(1) {
        memset(buf, 0, sizeof(buf));
        len = read(STDIN_FILENO, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, sizeof(buf));
        if (strstr(buf, "exit") != NULL) exit(0);
    }
}