/*********************************************************************************************************/
/*                                            Header-file
/*                                                 By
/*                                             Karl Sorsell
/*                                         
/*
/* The header-file for the client and the server. Contains function definitions and data type definitions
/*                              that are shared between the two programs.
/*********************************************************************************************************/





#ifndef header_h
#define header_h

#define PORT 5555  /*Defines what port the programs uses*/
#define messageLength 256 
#define MAXMSG 512
#define divisor 16 /*Defines what divisor to use in the checksum function*/

/*Defines the flags that are sent with each package*/
enum Flag
{
    SYN,
    SYNACK,
    ACK,
    NACK,
    NONE,
    TEARDOWN,
    TEARDOWN_ACK
};



/*Defines the package that is sent and recieved*/
typedef struct rtp_struct
{
    enum Flag flag;
    int id;
    int seq;
    int windowsize;
    int checksum;
    char data[messageLength];
    int repeatfunc;
} rtp;



struct sockaddr_in twhserver(rtp message, struct sockaddr_in serveraddr, int sockfd);

struct sockaddr_in server_setup(struct sockaddr_in serveraddr);

void twhclient(rtp message, struct sockaddr_in serveraddr, int sockfd);

struct sockaddr_in client_setup(struct sockaddr_in serveraddr);

void slidingwindow();

int checksum(char string[messageLength]);

void teardownClient(rtp message, struct sockaddr_in serveraddr, int sockfd);

void teardownServer(rtp message, struct sockaddr_in clientaddr, int sockfd);

void receivingFrames(rtp message);

void sendingFrames(rtp message);

#endif