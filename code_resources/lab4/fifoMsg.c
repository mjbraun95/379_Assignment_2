#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
		     
#define MAXLINE   132
#define MAXWORD    32

#define NF 3		 // number of fields in each message

#define MSG_KINDS 5
typedef enum { STR, INT, FLOAT, DONE, ACK } KIND;	  // Message kinds
char KINDNAME[][MAXWORD]= { "STR", "INT", "FLOAT", "DONE", "ACK" };

typedef struct { char  d[NF][MAXLINE]; } MSG_STR;
typedef struct { int   d[NF]; }          MSG_INT;
typedef struct { float d[NF]; }          MSG_FLOAT;

typedef union { MSG_STR  mStr; MSG_INT mInt; MSG_FLOAT mFloat; } MSG;

typedef struct { KIND kind; MSG msg; } FRAME;

// ------------------------------
// The WARNING and FATAL functions are due to the authors of
// the AWK Programming Language.

void FATAL (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
    fflush (NULL);
    exit(1);
}

void WARNING (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
}
// ------------------------------

// 
// ------------------------------
MSG composeMSTR (const char *a, const char *b, const char *c)
{
    MSG  msg;

    memset( (char *) &msg, 0, sizeof(msg) );
    strcpy(msg.mStr.d[0],a);
    strcpy(msg.mStr.d[1],b);
    strcpy(msg.mStr.d[2],c);
    return msg;
}    
// ------------------------------    
MSG composeMINT (int a, int b, int c)
{
    MSG  msg;

    memset( (char *) &msg, 0, sizeof(msg) );
    msg.mInt.d[0]= a; msg.mInt.d[1]= b; msg.mInt.d[2]= c;
    return msg;
}    
// ------------------------------
MSG  composeMFLOAT (float a, float b, float c)
{
    MSG  msg;

    memset( (char *) &msg, 0, sizeof(msg) );
    msg.mFloat.d[0]= a; msg.mFloat.d[1]= b; msg.mFloat.d[2]= c;
    return msg;
}
// ------------------------------
//####
void sendFrame (int fd, KIND kind, MSG *msg)
{
    FRAME  frame;

    assert (fd >= 0);
    memset( (char *) &frame, 0, sizeof(frame) );
    frame.kind= kind;
    frame.msg=  *msg;
    write (fd, (char *) &frame, sizeof(frame));
}
//####
FRAME rcvFrame (int fd)
{
    int    len; 
    FRAME  frame;

    assert (fd >= 0);
    memset( (char *) &frame, 0, sizeof(frame) );
    len= read (fd, (char *) &frame, sizeof(frame));
    if (len != sizeof(frame))
        WARNING ("Received frame has length= %d (expected= %d)\n",
		  len, sizeof(frame));
    return frame;		  
}
       
// ------------------------------
void printFrame (const char *prefix, FRAME *frame)
{
    MSG  msg= frame->msg;
    
    printf ("%s [%s] ", prefix, KINDNAME[frame->kind]);
    switch (frame->kind) {
    case STR:
        printf ("'%s' '%s' '%s'",
	   	 msg.mStr.d[0], msg.mStr.d[1], msg.mStr.d[2]);
        break;
     case INT:
         printf ("%d, %d, %d",
	   	  msg.mInt.d[0], msg.mInt.d[1], msg.mInt.d[2]);
         break;
      case FLOAT:
          printf ("%f, %f, %f",
	   	   msg.mFloat.d[0], msg.mFloat.d[1], msg.mFloat.d[2]);
          break;		   
      case ACK: case DONE:
          break;
      default:
          WARNING ("Unknown frame type (%d)\n", frame->kind);
	  break;
      }
      printf("\n");
}
// ------------------------------


//##
void do_client (int fifoCS, int fifoSC)
{
    FRAME  frame;
    MSG    msg;

    msg= composeMSTR ("Edmonton", "Red Deer", "Calgary");
    sendFrame (fifoCS, STR, &msg);
    frame= rcvFrame(fifoSC);  
    printFrame("received ", &frame);

    msg= composeMINT (10, 20, 30);
    sendFrame (fifoCS, INT, &msg);
    frame= rcvFrame(fifoSC);  
    printFrame("received ", &frame);

    msg= composeMFLOAT (10.25, 20.50, 30.75);
    sendFrame (fifoCS, FLOAT, &msg);
    frame= rcvFrame(fifoSC);  
    printFrame("received ", &frame);

    msg= composeMINT(0,0,0);
    sendFrame (fifoCS, DONE, &msg);
}

//##
void do_server (int fifoCS, int fifoSC)
{
    MSG    msg;
    FRAME  frame;

    while(1) {
       frame= rcvFrame(fifoCS);    	     
       printFrame ("received ", &frame);
       if (frame.kind == DONE) {printf ("Done\n"); return;}       

       sendFrame(fifoSC, ACK, &msg);       
    }
}

//####
int main (int argc, char *argv[])
{
    int fifoCS, fifoSC;
    
    if (argc != 2) 
    { 
        printf("Usage: %s [-c|-s]\n", argv[0]); 
        exit(0); 
    }

    if ( (fifoCS= open("fifo-cs", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed \n", argv[0], "fifo-cs");

    if ( (fifoSC= open("fifo-sc", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed \n", argv[0], "fifo-sc");
    
    if ( strstr(argv[1], "-c") != NULL) 
        do_client(fifoCS, fifoSC);
    if ( strstr(argv[1], "-s") != NULL) 
        do_server(fifoCS, fifoSC);

    close(fifoCS); close(fifoSC);
}

       