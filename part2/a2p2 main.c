#include <assert.h> 
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define NCLIENT 3
#define BUFSIZE 80
#define NUM_PACKET_TYPES 7
#define MAX_PACKET_TYPE_LEN 10

// TODO: Create a packet struct/object

// Source: eclass fifoMsg.c
typedef enum { PUT, GET, DELETE, GTIME, TIME, OK, ERROR } Packet_Type;	  // Message packet_types

typedef struct { char message[BUFSIZE]; } Message;
typedef struct { Packet_Type packet_type; Message message; } Packet;



//FIX
int get_packet_type(char* inputStr) {
char Packet_Type_Names[NUM_PACKET_TYPES][MAX_PACKET_TYPE_LEN] = {
    "put",
    "get",
    "delete",
    "gtime",
    "time",
    "OK",
    "ERROR"
};
    
    for (int i = 0; i < NUM_PACKET_TYPES; i++) {
        if (strcmp(inputStr, Packet_Type_Names[i]) == 0) {
            return i;
        }
    }
    
    return -1; // Not found
}
///FIX

//####
void sendPacket (int fd, Packet_Type packet_type, Message *message)
{
    Packet  packet;

    assert (fd >= 0);
    memset( (char *) &packet, 0, sizeof(packet) );
    packet.packet_type= packet_type;
    packet.message=  *message;
    write (fd, (char *) &packet, sizeof(packet));
}
//####
Packet rcvPacket (int fd)
{
    int    len; 
    Packet  packet;

    assert (fd >= 0);
    memset( (char *) &packet, 0, sizeof(packet) );
    len= read (fd, (char *) &packet, sizeof(packet));
    if (len != sizeof(packet))
        printf("Warning: Received packet has length= %d (expected= %d)\n",
		  len, sizeof(packet));
    return packet;		  
}

//Source: eClass
void spawn_client(int *fifo_cs_fd, int *fifo_sc_fd, char* client_ID, char* input_file) {
    FILE *fp;



    // Identify correct 2 FIFOs
    char fifo_cs_name[100], fifo_sc_name[100];
    // Eg. fifo_c1_s_fd
    sprintf(fifo_cs_name, "fifo_c%d_s", client_ID);
    // Eg. fifo_s_c1_fd
    sprintf(fifo_sc_name, "fifo_s_c%d", client_ID);

    // Open 2 FIFOs
    *fifo_cs_fd = open(fifo_cs_name, O_WRONLY | O_NONBLOCK );
    *fifo_sc_fd = open(fifo_sc_name, O_RDONLY | O_NONBLOCK );
    // TODO: Send packet to server
    fp = fopen(input_file, "r"); // open input file in read mode
    if (fp == NULL) {
        perror("Error: Unable to open input file\n");
        exit(EXIT_FAILURE);
    }

    // Initialize pollfd array
    // Initialize pollfd array to receive from all potential clients
    int client_rcv_fds[1] = {fifo_sc_fd};
    struct pollfd client_rcv_pfds[1];
    client_rcv_pfds[0].fd = client_rcv_fds[0];
    client_rcv_pfds[0].events = POLLIN;
    client_rcv_pfds[0].revents = 0;

// CLIENT ----------------------------------------------------
    // Client loop
    // Read input_file line-by-line
    int timeout = -1; //TODO?: Change to 0 or 1
    char packet_buffer[BUFSIZE];
    while (fgets(packet_buffer, BUFSIZE, fp) != NULL) {
        
        // TODO: Create + send a packet
        // Initialize message
        Message message;
        strcpy(message.message, packet_buffer);

        // Initialize packet_type
        Packet_Type packet_type;
        char* token = strtok(packet_buffer, " ");
        char first_word[MAX_PACKET_TYPE_LEN];   
        strcpy(first_word, token);
        if (first_word[0] == "#") {
            printf("Skipping has line...\n");
            continue;
        }

        int index = get_packet_type(first_word);
        switch (index) {
            case PUT:
                packet_type = PUT;
                printf("Transmitted (src= client:?) PUT\n");
            case GET:
                packet_type = GET;
                printf("Transmitted (src= client:?) GET\n");
            case DELETE:
                packet_type = DELETE;              
                printf("Transmitted (src= client:?) DELETE\n");
            case GTIME:
                packet_type = GTIME;
                printf("Transmitted (src= client:?) GTIME\n");
            case TIME:
                packet_type = TIME;
                printf("Transmitted (src= client:?) TIME\n");
            case OK:
                packet_type = OK;
                printf("Transmitted (src= client:?) OK\n");
            case ERROR:
                packet_type = ERROR;
                printf("Transmitted (src= client:?) ERROR\n");
            default:
                //Invalid index
                perror("Error: Invalid packet type (index)\n");                                                                      
        }
        // Send the packet
        printf("Attempting to send packet from client... \n[%s]\n", packet_buffer);
        sendPacket(fifo_cs_fd, packet_type, &message);
        
        // // TODO?: Wait until the server's FIFO becomes ready for writing
        // int num_events = poll(client_rcv_fds, 1, timeout);
        // // Check that poll worked properly
        // if (num_events == -1) {
        //     perror("Error: Error polling on the client side.\n");
        //     exit(EXIT_FAILURE);
        // }
        // // Check for event from server
        // if (client_rcv_pfds[0].revents & POLLIN) {
        //     printf("client %d received event from server", client_ID);
        // }

        // Write packet buffer to server
        // printf("Attempting to send packet from client... \n[%s]\n", packet_buffer);
        // ssize_t nread = strlen(packet_buffer);
        // ssize_t buffer_len = write(fifo_cs_fd, packet_buffer, nread);
        // if (buffer_len < 0 && errno != EAGAIN) {
        //     perror("Error: FIFO writing error\n");
        //     exit(EXIT_FAILURE);
        // }
        // Check if success
        // if (buffer_len > 0) {
        //     printf("Send succesful! \n[%s]\n", packet_buffer);
        // }

    }
    // Close fp and FIFOs
    fclose(fp);
    close(fifo_cs_fd);
    close(fifo_sc_fd);
    
    return EXIT_SUCCESS;
}

//Source: eClass
        // Source: eClass: poll.c
void launch_server(int *fifo_c1_s_fd, int *fifo_c2_s_fd, int *fifo_c3_s_fd, int *fifo_s_c1_fd, int *fifo_s_c2_fd, int *fifo_s_c3_fd) {
    char packet_buffer[BUFSIZE];
    // Open all 6 FIFOs
    // To server
    fifo_c1_s_fd = open("fifo-1-0", O_RDONLY | O_NONBLOCK );
    fifo_c2_s_fd = open("fifo-2-0", O_RDONLY | O_NONBLOCK );
    fifo_c3_s_fd = open("fifo-3-0", O_RDONLY | O_NONBLOCK );
    // To client
    fifo_s_c1_fd = open("fifo-0-1", O_WRONLY | O_NONBLOCK );
    fifo_s_c2_fd = open("fifo-0-2", O_WRONLY | O_NONBLOCK );
    fifo_s_c3_fd = open("fifo-0-3", O_WRONLY | O_NONBLOCK );    

    // Source: eClass: poll.c
    int server_rcv_fds[NCLIENT] = {fifo_c1_s_fd, fifo_c2_s_fd, fifo_c3_s_fd};

    // Initialize pollfd array to receive from all potential clients
    struct pollfd server_rcv_pfds[NCLIENT];
    for (int i = 0; i < NCLIENT; i++) {
        server_rcv_pfds[i].fd = server_rcv_fds[i];
        server_rcv_pfds[i].events = POLLIN;
        server_rcv_pfds[i].revents = 0;
    }

    int timeout = 0;

    // TODO: Let the client know that the server is running

// SERVER ----------------------------------------------------
    Packet received_packet;
    // Server loop
    while (1)
    {
        received_packet = rcvPacket(fifo_c3_s_fd);
        if (strlen(received_packet.message.message) != 0) {
            printf("received_packet.message.message: %s\n", received_packet.message.message);
            printf("  Packet type: %d\n", received_packet.packet_type);
            // printf("Event received from client[%d]\n", client_id);

            // Receive packet
                    
            // printf("  Message: %s", received_packet.message);
            switch (received_packet.packet_type) {
                case PUT:
                    //TODO: Print formatting
                    printf("Received (src= server) (PUT: %s)\n");
                case GET:
                    //TODO: Print formatting
                    printf("Received (src= server) (GET:)\n");
                case DELETE:
                    //TODO: Print formatting
                    printf("Received (src= server) (DELETE:)\n");
                case GTIME:
                    //TODO: Print formatting
                    printf("Received (src= server) (GTIME:)\n");
                case TIME:
                    //TODO: Print formatting
                    printf("Received (src= server) (TIME:)\n");
                case OK:
                    //TODO: Print formatting
                    printf("Received (src= server) (OK)\n");
                case ERROR:
                    //TODO: Print formatting
                    printf("Received (src= server) (ERROR: )\n");
                default:
                    //Invalid index
                    perror("Error: Invalid packet type (index)\n");    
            }
 
        }

    }	
}

// TODO: execute packets
// TODO: Identify command
// TODO: Execute command
// TODO: Reply with OK or ERROR w/description


// Source: eClass, fifoMsg.c
void FATAL (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
    fflush (NULL);
    exit(1);
}



//Source: eClass
int main (int argc, char *argv[])
{
    // Spawn FIFOs
    // int return_value = system("sh spawn_fifos.sh");
    // if (return_value != 0) {
    //     printf("FIFO creation failed.\n");
    //     exit(0);
    // }

    int fifo_c1_s_fd, fifo_c2_s_fd, fifo_c3_s_fd, fifo_s_c1_fd, fifo_s_c2_fd, fifo_s_c3_fd;
    


    //Check if FIFOs are being opened properly?
    if ( (fifo_c1_s_fd= open("fifo-1-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c1_s_fd\n");
    if ( (fifo_c2_s_fd= open("fifo-2-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c2_s_fd\n");
    if ( (fifo_c3_s_fd= open("fifo-3-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c3_s_fd\n");
    if ( (fifo_s_c1_fd= open("fifo-0-1", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c1_fd\n");
    if ( (fifo_s_c2_fd= open("fifo-0-2", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c2_fd\n");
    if ( (fifo_s_c3_fd= open("fifo-0-3", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c3_fd\n");
    
    //Client check
    if ( strcmp(argv[1], "-c") == 0) {
        //Arg count check
        if (argc != 4) 
        { 
            printf("Usage: ./a2p2 -c client_id input_filename\n"); 
            exit(0); 
        }
        //Client ID check
        if (strcmp(argv[2], "1") == 0) {
            spawn_client(&fifo_c1_s_fd, &fifo_s_c1_fd, argv[2], argv[3]);
        }
        else if (strcmp(argv[2], "2") == 0) {
            spawn_client(&fifo_c1_s_fd, fifo_s_c1_fd, argv[2], argv[3]);
        }
        else if (strcmp(argv[2], "3") == 0) {
            spawn_client(&fifo_c1_s_fd, fifo_s_c1_fd, argv[2], argv[3]);
        }
        else {
            printf("Usage: %s [-c|-s]\n", argv[0]); 
            exit(0); 
        }

    }
        
    //Server check
    if ( strcmp(argv[1], "-s") == 0) {
        //Arg count check
        if (argc != 2) 
        { 
            printf("Usage: ./a2p2 -s\n"); 
            exit(0); 
        }
        launch_server(&fifo_c1_s_fd, &fifo_c2_s_fd, &fifo_c3_s_fd, &fifo_s_c1_fd, &fifo_s_c2_fd, &fifo_s_c3_fd);
    } 

    close(fifo_c1_s_fd); 
    close(fifo_c2_s_fd);
    close(fifo_c3_s_fd);
    close(fifo_s_c1_fd);
    close(fifo_s_c2_fd);
    close(fifo_s_c3_fd);
}