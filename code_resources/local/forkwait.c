/*
# ------------------------------------------------------------
# forkwait.c - a simple program illustrating the fork() operation
#
#     Compilation:
#     Linux		gcc  forkwait.c -o forkwait
#
#     Usage:		forkwait
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]
# ------------------------------------------------------------
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NLINES 40
int i, pid, status;

int main (void) {
  pid = fork ();
  if (pid == 0) {             /* the child process */
    for (i=1; i < NLINES; i++)
      printf ("%d. I am the child! \n", i);
  } else {                   /* the parent process */
    for (i= NLINES; i > 0; i--)
      printf ("%d. I am the parent!\n", i);
    wait (&status);
  }
  return 0;
}
