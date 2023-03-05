#include <assert.h> 
#include <assert.h> 
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 80
#define MAXCMD 10
#define MAXPATH 50
#define MAXWORD 32
#define MAX_PACKET_TYPE_LEN 6
#define NCLIENT 3
#define NUM_PACKET_TYPES 7

typedef enum { PUT, GET, DELETE, GTIME, TIME, OK, ERROR } Packet_Type;

typedef struct { char message[MAXWORD]; } Message;
typedef struct { Packet_Type packet_type; Message message; } Packet;

char* process_message(Message received_message) {
   char* file_name = malloc(sizeof(char) * MAXWORD);
   strcpy(file_name, received_message.message);
   return file_name;
}

//Source: fifoMsg.c
void send_packet (int fd, Packet_Type packet_type, Message *message)
{
    Packet  packet;
    assert (fd >= 0);
    memset( (char *) &packet, 0, sizeof(packet) );
    packet.packet_type= packet_type;
    packet.message=  *message;
    write (fd, (char *) &packet, sizeof(packet));
}

//Source: fifoMsg.c
Packet receive_packet (int fd)
{
    int    len; 
    Packet  packet;
    assert (fd >= 0);
    memset( (char *) &packet, 0, sizeof(packet) );
    len= read (fd, (char *) &packet, sizeof(packet));
    if (len != sizeof(packet))
        printf("Warning: Received packet has length= %d (expected= %d)\n", len, sizeof(packet));
    return packet;		  
}

void remove_new_line(char* str) {
    size_t len_str = strcspn(str, "\n");
    str[len_str] = '\0';
}

//Source: eClass
void spawn_client(int *fifo_cs_fd, int *fifo_sc_fd, char* client_ID, char* input_file) {
    // Identify correct 2 FIFOs
    char fifo_cs_name[100], fifo_sc_name[100];
    sprintf(fifo_cs_name, "fifo-%s-0", client_ID);
    sprintf(fifo_sc_name, "fifo-0-%s", client_ID);

    // Open 2 FIFOs
    *fifo_cs_fd = open(fifo_cs_name, O_WRONLY | O_NONBLOCK );
    *fifo_sc_fd = open(fifo_sc_name, O_RDONLY | O_NONBLOCK );

    // Open input file in read mode
    FILE *fp;
    fp = fopen(input_file, "r"); 
    if (fp == NULL) {
        perror("Error: Unable to open input file\n");
        exit(EXIT_FAILURE);
    }

    // Initialize pollfd array
    int client_rcv_fds[1] = {fifo_sc_fd};
    struct pollfd client_rcv_pfds[1];
    client_rcv_pfds[0].fd = client_rcv_fds[0];
    client_rcv_pfds[0].events = POLLIN;
    client_rcv_pfds[0].revents = 0;
    int timeout = -1; //TODO?: Change to 0 or 1

    // Client loop
    // Read input_file line-by-line
    char packet_buffer[BUFSIZE];
    char arg1[1], arg2[MAX_PACKET_TYPE_LEN], arg3[MAXWORD];
    char* token;
    while (fgets(packet_buffer, BUFSIZE, fp) != NULL) {
        Packet_Type packet_type;
        Message message;

        // Check for # lines
        if (packet_buffer[0] == '#') {
            printf("debug: #. Skipping line...%s\n", packet_buffer);
            continue;
        }

        // Remove newline from buffer
        remove_new_line(packet_buffer);

        // Parse buffer into arguments
        token = strtok(packet_buffer, " ");
        strcpy(arg1, token);
        token = strtok(NULL, " ");
        strcpy(arg2, token);
        token = strtok(NULL, " ");
        if (token) {
            // Check 3rd arg exists before copying
            strcpy(arg3, token);
        }

        // Assign packet type
        int i = 0;
        while (arg2[i]) {
            // Convert arg2 to uppercase
            arg2[i] = tolower(arg2[i]);
            i++;
        }
        if (strcmp(arg2, "put") == 0) packet_type = PUT;
        else if (strcmp(arg2, "get") == 0) packet_type = GET;
        else if (strcmp(arg2, "delete") == 0) packet_type = DELETE;              
        else if (strcmp(arg2, "gtime") == 0) packet_type = GTIME;
        else if (strcmp(arg2, "delay") == 0) sleep(atoi(arg3)/1000);
        else if (strcmp(arg2, "quit") == 0) break;
        else perror("Error: Invalid arg2 in client\n");

        // Assign message
        if (packet_type != PUT && packet_type != GET && packet_type != DELETE && packet_type != GTIME) continue; // No packets sent for delay case
        if (packet_type == PUT || packet_type == GET || packet_type == DELETE) {
            // Load object into message
            strcpy(message.message, arg3);
        } 

        // Send packet
        send_packet(*fifo_cs_fd, packet_type, &message);
        printf("Transmitted (src= client:%s) %s\n", client_ID, packet_type);

        // TODO: Wait for server reply
        while (1) {
            int num_events = poll(client_rcv_fds, 1, timeout);
            // Check that poll worked properly
            if (num_events == -1) {
                perror("Error: Error polling on the client side.\n");
                exit(EXIT_FAILURE);
            }
            // Check for event from server
            if (client_rcv_pfds[0].revents & POLLIN) {
                printf("client %d received event from server\n", client_ID);
                break;
            }
        }


    }
    // Close fp and FIFOs
    fclose(fp);
    close(fifo_cs_fd);
    close(fifo_sc_fd);
    
    return EXIT_SUCCESS;
}

//Source: eClass
void launch_server(int *fifo_c1_s_fd, int *fifo_c2_s_fd, int *fifo_c3_s_fd, int *fifo_s_c1_fd, int *fifo_s_c2_fd, int *fifo_s_c3_fd) {
    clock_t start_time = clock();
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
    int fifo_fds[NCLIENT] = {fifo_c1_s_fd, fifo_c2_s_fd, fifo_c3_s_fd};

    // Initialize pollfd array to receive from all potential clients
    struct pollfd pollfds[NCLIENT+1];
    for (int i = 1; i < NCLIENT+1; i++) {
        pollfds[i].fd = fifo_fds[i-1];
        pollfds[i].events = POLLIN;
        pollfds[i].revents = 0;
    }

    int timeout = 0;

    // Server loop
    while (1)
    {
        // Source: eClass: poll.c
        int num_events = poll(pollfds, NCLIENT, timeout);
        if (num_events == -1) {
            perror("Error: Poll failed\n");
            exit(EXIT_FAILURE);
        }
        for (int client_id = 1; client_id < NCLIENT+1; client_id++) {
            // Check each pollfd for any received events
			if (pollfds[client_id].revents & POLLIN) 
			{
                // Event received
                printf("Event received from client[%d]\n", client_id);

                // Choose corresponding FIFO
                Packet received_packet;
                int *active_cs_fifo;
                int *active_sc_fifo;
                if (client_id > NCLIENT || client_id <= 0) perror("Error: Invalid client_id (server)\n");
                // if (client_id == 1) received_packet = receive_packet(fifo_c1_s_fd);
                // if (client_id == 2) received_packet = receive_packet(fifo_c2_s_fd);
                // if (client_id == 3) received_packet = receive_packet(fifo_c3_s_fd);
                if (client_id == 1) {active_cs_fifo = fifo_c1_s_fd; active_sc_fifo = fifo_s_c1_fd;}
                if (client_id == 2) {active_cs_fifo = fifo_c2_s_fd; active_sc_fifo = fifo_s_c2_fd;}
                if (client_id == 3) {active_cs_fifo = fifo_c3_s_fd; active_sc_fifo = fifo_s_c3_fd;}
                received_packet = receive_packet(active_cs_fifo);
                Packet_Type received_type = received_packet.packet_type;
                Message received_message = received_packet.message;
                printf("  debug: Packet type: %s\n", received_type);
                printf("  debug: Message: %s\n", received_message.message);
                char target_path_server[MAXPATH] = "server_directory/";
                char target_path_client[MAXPATH] = "client_directory/";
                
                if (received_type == PUT) {
                    // PUT: Client upload
                    char *file_name = process_message(received_message);
                    strcat(target_path_server, file_name);
                    // If the object already exists in the server directory, throw an error
                    if (access(target_path_server, F_OK) != -1) {
                        char reply[MAXWORD] = "ERROR: object already exists";
                    }
                    // If not, make it exist
                    else {
                        char command[MAXCMD + MAXWORD];
                        sprintf(command, "touch %s", target_path_server);
                        system(command);
                    }
                    printf("Transmitted (src= client:%s) (%s: %s)\n", client_id, received_type, received_message.message);
                } else if (received_type == GET) {
                    // GET: Client download
                    char *file_name = process_message(received_message);
                    strcat(target_path_server, file_name);
                    // If the object doesn't exist in the server directory, throw an error
                    if (access(target_path_server, F_OK) == -1) {
                        char reply[MAXWORD] = "ERROR: object not found\n";
                    }
                    // If it DOES exist in the server, make it exist in the client
                    else {
                        char command[MAXCMD + MAXWORD];
                        sprintf(command, "touch %s", target_path_client);
                        system(command);
                    }
                    printf("Received (src= server:%s) (%s: %s)\n", client_id, received_type, received_message.message);
                } else if (received_type == DELETE) {
                    printf("Transmitted (src= client:%s) (%s: %s)\n", client_id, received_type, received_message.message);
                } else if (received_type == GTIME) {
                    clock_t end_time = clock();
                    double time_since_server_start = (((double) (end_time - start_time)) / CLOCKS_PER_SEC * 1000.0);
                    printf("Transmitted (src= server) (TIME: %d)\n", time_since_server_start);
                } else {
                    perror("Error: Invalid received packet type (index)\n");    
                }
                // TODO: Reply with OK or ERROR w/description
			}	
		}
    }
}

// Source: eClass: fifoMsg.c
void FATAL (const char *fmt, ... )
{
    va_list  ap;
    fflush (stdout);
    va_start (ap, fmt);  vfprintf (stderr, fmt, ap);  va_end(ap);
    fflush (NULL);
    exit(1);
}



//Source: eClass: fifoMsg.c
int main (int argc, char *argv[])
{
    int fifo_c1_s_fd, fifo_c2_s_fd, fifo_c3_s_fd, fifo_s_c1_fd, fifo_s_c2_fd, fifo_s_c3_fd;
    
    // Open FIFOs
    if ( (fifo_c1_s_fd = open("fifo-1-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c1_s_fd\n");
    if ( (fifo_c2_s_fd = open("fifo-2-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c2_s_fd\n");
    if ( (fifo_c3_s_fd = open("fifo-3-0", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_c3_s_fd\n");
    if ( (fifo_s_c1_fd = open("fifo-0-1", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c1_fd\n");
    if ( (fifo_s_c2_fd = open("fifo-0-2", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c2_fd\n");
    if ( (fifo_s_c3_fd = open("fifo-0-3", O_RDWR)) < 0)
        FATAL ("%s: open '%s' failed ", argv[0], "fifo_s_c3_fd\n");
    
    // Check args
    // Client check
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

    // Close FIFOs
    close(fifo_c1_s_fd); 
    close(fifo_c2_s_fd);
    close(fifo_c3_s_fd);
    close(fifo_s_c1_fd);
    close(fifo_s_c2_fd);
    close(fifo_s_c3_fd);
}