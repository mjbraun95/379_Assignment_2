/*
# ------------------------------------------------------------
# signal.c - tests the signal() system call
#
#     Compilation:
#     Linux		gcc  signal.c -o signal
#
#     Usage:		signal
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]
# ------------------------------------------------------------
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int i;

void quit(int code) {
     signal(SIGQUIT,quit);
     fprintf(stderr, "\nInterrupt (code= %d, i= %d)\n", code, i);
}

int main (void) {
     signal(SIGQUIT,quit);
     for (i= 0; i < 9e7; i++)
       if (i % 10000 == 0) {putc('.', stderr); sleep(1);}
     return(0);
}
