/*
# ------------------------------------------------------------
# sigact.c - illustrates the use of sigaction()
#
#     Compilation:
#     Linux		gcc  sigact.c -o sigact
#
#     Usage:		sigact
#			Program catches SIGQUIT signals
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]
# ------------------------------------------------------------
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int i;
struct sigaction oldAct, newAct;

void quitHandler(int sigNo, siginfo_t *sigInfo, void *a)
{
     if (sigInfo != NULL) fprintf(stderr, "\n signo= %d, sender= %d, i= %d)\n",
		                          sigNo, sigInfo->si_pid, i);
     else                 fprintf(stderr, "\n signo= %d, i= %d)\n", sigNo, i);
}

int main (void) {

     newAct.sa_sigaction= quitHandler; 
     sigemptyset(&newAct.sa_mask);
     newAct.sa_flags |= SA_SIGINFO;

     printf("pid= %d \n", getpid());
     printf ("oldAct: size= %ld, add= %p\n", sizeof(oldAct), &oldAct);	
     printf ("quitHandler: add= %p, %p\n", quitHandler, &quitHandler);	
     printf ("sa_flags= %x\n", newAct.sa_flags);

     if (sigaction (SIGQUIT, &newAct, &oldAct) < 0)
	perror("Can't install new handler");

     for (i= 0; i < 9e7; i++)
       if (i % 10000 == 0) { putc('.', stderr); sleep(1); }

     sigaction(SIGQUIT,&oldAct,&newAct); 
     return(0);
}


