/*
# ------------------------------------------------------------
# sigsetjump.c - a program to test sigsetjmp() and siglongjmp() calls
#
#     Compilation:
#     Linux		gcc  sigsetjump.c -o sigsetjump
#
#     Usage:		sigsetjump
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]
# ------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

jmp_buf return_pt;
int i;

void quit(int code) {
     char answer;
     fputs("\n restart the loop?", stdout); answer= fgetc(stdin);
     if (answer == 'y') siglongjmp(return_pt,1);
}

int main (void) {
  if (sigsetjmp(return_pt,1) == 0) signal(SIGQUIT,quit);
     printf("\nloop begins:\n");
     for (i= 0; i < 9e7; i++)
       if (i % 10000 == 0) { putchar('.'); fflush(stdout); sleep(1);}
     puts( "\nFinished!");
     return 0;
}
