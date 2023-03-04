/*
# ------------------------------------------------------------
# poll.c - a server process forks N client processes and uses
#          pipes for communication
#
#     Compilation:
#     Linux		gcc  poll.c -o poll
#
#     Usage:		poll
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]
# ------------------------------------------------------------
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>          
#include <poll.h>	

#define	N	3

int	fd[N][2], pid[N];

int  fork_and_write (int id) {
    int		i;
    char  	buf[20];
    pid_t	pid;

    if ( (pid= fork()) < 0 )	{ perror ("fork error \n"); exit (1); }
    if (pid > 0)		return pid;

    buf[0]= '0' + id;

    close (fd[id][0]);			 /* child won't read */

    for (i=0; i < 10; i++)  {
    		// printf ("fork_and_write: writing to fd= %d\n", fd[id][1]);

    	if (write(fd[id][1], buf, 1) < 0) 
    		{ perror ("pipe write error \n"); exit (1); }
   	sleep(1);
    }    	

    buf[0]= -1; write(fd[id][1], buf, 1);
    exit (0);
}

/* ------------------------------ */
int main (int argc, char **argv) {
    int			i, len, rval, writerId, n_writers, dotCount;
    int			timeout;
    int			bufsize= 80;
    char		buf[bufsize];
    int			done[N];
    struct pollfd 	pollfd[N];


    for (i= 0; i < N; i++)
    	if (pipe(fd[i]) < 0) { perror ("pipe error \n"); exit (1); }

    for (i= 0; i < N; i++)  pid[i]= fork_and_write(i);
	 //   printf ("debug: main: forking writer %d, fd0= %d, fd1= %d\n",
    	 // 		i, fd[i][0], fd[i][1]);

    for (i= 0; i < N; i++) {
    	close (fd[i][1]);			/* parent won't write */
    	done[i]= 0;
    	pollfd[i].fd= fd[i][0];
		pollfd[i].events= POLLIN;
		pollfd[i].revents= 0;
		//printf("debug: setting writer= %d, done= %d, revents= %x\n",
		//i, done[i], pollfd[i	].events);
    }	

    timeout= 0;

    n_writers= N; dotCount= 0;

    while (n_writers > 0) {

    	rval= poll (pollfd, N, timeout);
		 // printf ("rval= %d \n", rval);
    	    
		for (writerId= 0; writerId < N; writerId++) {

			//printf("debug: writer= %d, done= %d, revents= %x\n",
			//	writerId, done[writerId], pollfd[writerId].revents);
			if ( done[writerId] == 0  && (pollfd[writerId].revents & POLLIN) )
			{
				len= read (fd[writerId][0], buf, bufsize);
					//printf ("debug: writer= %d, len= %d\n", writerId,len);
				for (i= 0; i < len; i++) 
				{
					if (buf[i] == -1)
					{ 
					n_writers--; done[writerId]= 1; break;
					}
					else printf("[%c] ", buf[0]);
				}

			}	
		}
		if ( ++dotCount % 10000 == 0 ) putc('.', stderr);
	}	  	      	    	
	printf("\n");
}

                         	
