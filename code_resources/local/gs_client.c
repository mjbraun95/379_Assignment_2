/*
# ------------------------------------------------------------
# gs_client.c - a simple client using SOCK_STREAM sockets
#
#     Compilation:
#     	Linux 		gcc gs_client.c -o gs_client
#
#     Usage:		gs_client  remotehost  service
#
#     Features:		- use of setrlimit() to limit the CPU time
#			  note: time spent while sleeping, or waiting for input
#			  is not user CPU time
#			- use of getservbyname() and gethostbyname()
#			- reading and writing to the same socket
#			- use of I/O multiplexing to read user input
#			  from the stdin, and responses from the socket
#
# [U. of Alberta, CMPUT379, E.S. Elmallah]	
# ------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>		// required by setrlimit()
#include <sys/resource.h>	// required by setrlimit()

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>	// defines struct hostent, struct servent,
			// and prototypes for obtaining host and
			// service information
#include <poll.h>

#define	CPU_LIMIT	10

int main (int argc, char **argv) {
    
    int			i, rval, timeout, s;
    char		buf[80];
    struct rlimit	cpuLimit;
    struct pollfd	pfd[2];
    struct sockaddr_in	server;
    struct servent	*sp;			/* service entity */
    struct hostent	*hp;			/* host entity    */ 
    
    FILE		*sfpin;

    // check for at least two arguments

    if (argc < 3) {
    	fprintf (stderr, "Usage: %s hostname service\n", argv[0]);
    	fprintf (stderr, "       e.g., %s ui03.cs.ualberta.ca  ssh\n",
				 argv[0]);
    	exit (1);
    }    	

    // set a cpu limit

    cpuLimit.rlim_cur= cpuLimit.rlim_max= CPU_LIMIT;

    if (setrlimit (RLIMIT_CPU, &cpuLimit) < 0 ) {
    	fprintf (stderr, "%s: can't set cpu limit \n", argv[0]);
	exit (1);
    }    	
    getrlimit (RLIMIT_CPU, &cpuLimit);
    printf ("cpuLimit: current (soft)= %ld, maximum (hard)= %ld \n",
    		cpuLimit.rlim_cur, cpuLimit.rlim_max);
    

    // locate the service definition

    sp= getservbyname (argv[2], "tcp");

    if (sp == (struct servent *) NULL) {
	fprintf (stderr, "%s: unknown tcp service \n", argv[0]);
	exit (1);
    }
    
    // lookup the specified host

    hp= gethostbyname(argv[1]);
    if (hp == (struct hostent *) NULL) {
	fprintf (stderr, "%s: unknown host '%s'\n", argv[0], argv[1]);
	exit (1);
    }

    // put the host's address, and type into a socket structure;
    // first, clear the structure, then fill in with the IP address
    // of the foreign host, and the port number at which the remote
    // server listens
        
    memset ((char *) &server, 0, sizeof server);
    memcpy ((char *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family= hp->h_addrtype;
    server.sin_port= sp->s_port;
    
    // create a socket, and initiate a connection

    s= socket(hp->h_addrtype, SOCK_STREAM, 0);
    if (s < 0) {
	fprintf (stderr, "%s: socket \n", argv[0]);
	exit (1);
    } 

    // if the socket is unbound at the time of connect(); the system
    // automatically selects and binds a name to the socket
    
    if (connect(s, (struct sockaddr *) &server, sizeof server) < 0) {
	fprintf (stderr, "%s: connect\n", argv[0]);
	exit (1);
    }

    // we may also want to perform STREAM I/O on the socket

    if ((sfpin= fdopen(s, "r")) < 0) {
    	fprintf (stderr, "%s: converting s to FILE* \n", argv[0]);
    	exit (1);
    }    	    

    // prepare for nonblocking I/O polling from the socket and the keyboard

    timeout=  0;
    pfd[0].fd= s;   pfd[1].fd=      STDIN_FILENO;
    pfd[0].events=  pfd[1].events=  POLLIN;
    pfd[0].revents= pfd[1].revents= 0;
    	
    while (1) {
	rval= poll (pfd, 2, timeout);

	if (pfd[0].revents & POLLIN) {		/* check socket */
	    if (fgets(buf, 80, sfpin) != NULL) printf("%s", buf);
	}

	if (pfd[1].revents & POLLIN) {		/* stdin input */
	    fgets(buf, 80, stdin);
	    write(s, buf, strlen(buf));
	}

    }
    printf("\n"); close(s); return 0;
}
