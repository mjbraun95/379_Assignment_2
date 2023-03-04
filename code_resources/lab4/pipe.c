/*
    APUE 15.5
*/

#define MAXLINE 12
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int     n;
    int     fd[2];
    pid_t   pid;
    char    line[MAXLINE];

    /* Create a pipe */
    if (pipe(fd) < 0)
        printf("pipe error");

    /* Create a process */
    if ((pid = fork()) < 0) {
        printf("fork error");
    } else if (pid > 0) {                       /* parent */
        close(fd[0]);                           /* close fd[0] No reading */
        write(fd[1], "hello world\n", 12);      /* write fd[1] */
    } else {                                    /* child */
        close(fd[1]);                           /* close fd[1] No writing*/
        n = read(fd[0], line, MAXLINE);         /* read from fd[0]*/
        write(STDOUT_FILENO, line, n);          /* write to stdout */
    }
    exit(0);
}