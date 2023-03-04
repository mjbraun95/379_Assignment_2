/*
Process A first opens a FIFO in the O_RDWR mode and waits for input 
from the user (without writing to the FIFO). 
Subsequently, while A is waiting for the user input, process B attempts
to open the FIFO in a O_RDONLY mode.
*/

// See what happens when you give | O_NONBLOCK option


/****************
 Example program to demonstrate use of FIFOs (named pipes)
 https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/lab5-fifo.c
 ****************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>   // mkfifo
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

void errexit(char *errMsg){
  printf("\n About to exit: %s", errMsg);
  fflush(stdout);
  exit(1);
}

int main()
{
    int ret;
    pid_t pid;
    int value;
    char fifoName[]="/tmp/testfifo";
    char errMsg[1000];
    int cfp;
    int pfp;
    char buffer[5];
    char input_buffer[5];

    ret = mkfifo(fifoName, 0600); 
    /* 0600 gives read, write permissions to user and none to group and world */
    if(ret < 0){
      sprintf(errMsg,"Unable to create fifo: %s",fifoName);
      errexit(errMsg);
    }

    pid=fork();
    if(pid == 0){
	    /* child -- open the named pipe and write an integer to it */
      printf("I'm process A trying to open\n");
      cfp = open(fifoName, O_RDWR | O_NONBLOCK );
      if(cfp == -1) 
	      errexit("Unable to open fifo for read/write");
      /* process waits for input from user */
      printf("Enter Input for process A:\n");
      scanf("%s", input_buffer);

      printf("I'm process A trying to write\n");
      ret=write(cfp,input_buffer,6);
      // fflush(cfp);
      exit(0);
    } 

    else{
      /* parent - open the named pipe and read an integer from it */
      printf("I'm process B trying to open\n");
      pfp = open(fifoName, O_RDONLY |  O_NONBLOCK  );
      if(pfp == -1){
        // printf("Error No: %d", errno);
	      errexit("Unable to open fifo for reading");
      }

      printf("I'm process B trying to read\n");
      ret=read(pfp,&buffer,5);
      if(ret < 0) 
	      errexit("Error reading from named pipe");
      close(pfp);
      printf("I'm process B. Received value '%s' from Process A on fifo \n", buffer);
      unlink(fifoName); /* Delete the created fifo */
      exit(0);
    }
}
