/*********************************************************************************************************/
/*                                               Client
/*                                                 By
/*                                             Karl Sorsell
/*
/* The client-part of the program. Start the client side AFTER starting the server.
/* Will initiate a three-way handshake with the server and then start sending the packages. When all the
/* ACKs have been received by the client a call for teardown will be made by the client and the teardown
/* sequence will be started. After completion of teardown both programs will terminate and the time taken
/*                                 from start to finish will be printed.
/*********************************************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>    /*for time*/
#include <sys/ioctl.h> /*for ioctl function*/
#include "header.h"
#include <stdbool.h>    /*for using boolean statements*/

#define WS 3                /*For defining how many frames in sliding window*/
#define NumOfFrames 19      /*For defining number of frames to be sent*/

#define goBackN 1
#define selectiveRepeat 2
int repeatfunc = goBackN; /*By writing goBackN or selctiveRepeat, you choose which repeat protocol*/


int windowhigh = WS ; // windowhigh and windowlow are use to increment the sliding window every time an ack is received and keeo the window size the same.
int windowlow = 0;
int serverId, clientId; /*Unique id for both client and server, used in verifing that the package is from the right sender and ACK is from the right sender*/
int OutputStatus = 0; /*selects what outputs to give in the terminal*/


struct sockaddr_in serveraddr; /*For saving servers information*/
int sockfd;     /*for socket*/

time_t *start, *end, *current; /*variables for keeping time*/
/*Defining timeout times*/
float timeout_TWH_TEARDOWN = 0.1; /*Defines how long timeout should be*/
float timeout = 0.01;

bool seqarr[NumOfFrames]; /*Array for keeping track of what ACKs have been recieved*/
int status;



/*Function for simulating random errors when sending frames.
/*Needed for showing that the repeat protocol works*/
int errorfunc()
{
    int random = (rand() % 10) + 1;

    if (random <= 8)
    {
        return 1;
    }
    else
        return 0;
}
/*Function for reading from socket and printing what it is*/

rtp ReceivFunc(rtp message)
{
    int nOfBytes;
    /*Reads from socket*/
    nOfBytes = recvfrom(sockfd, &message, MAXMSG, MSG_WAITALL, NULL, NULL);
    if (nOfBytes < 0)
    {
        perror("Could not read data from client\n");
        exit(EXIT_FAILURE);
    }
    if (message.flag > 0 && message.flag < 7)
    {
/*prints what type of package it was*/
        switch (message.flag)
        {
        case 1:
            if (OutputStatus == 0)
            {
                printf("Reciving SYNACK \n");
            }
            else
            {
                printf("Reciving SYNACK on package %d\n", message.seq);
            }
            break;

        case 2:
            if (OutputStatus == 0)
            {
                printf("Reciving ACK \n");
            }
            else
            {
                printf("Reciving ACK on package %d\n", message.seq);
            }
            break;

        case 3:
            if (OutputStatus == 0)
            {
                printf("Reciving NACK \n");
            }
            else
            {
                printf("Reciving NACK package %d\n", message.seq);
            }
            break;

        case 6:
            if (OutputStatus == 0)
            {
                printf("Reciving TEARDOWN_ACK \n");
            }
            else
            {
                printf("Reciving TEARDOWN_ACK package %d\n", message.seq);
            }
            break;
        }
        /*Prints at what time it was recieved*/
        *current = clock();
        double passed = (double)(*current - *start) / CLOCKS_PER_SEC;
        //printf("Time elapsed: %f\n\n", passed);
    }
        /*Returns the package*/
    return message;
}
/*Function for writing to socket (sending package)*/
void SendPackage(rtp message)
{
    int SendStatus = errorfunc();
    int corruptSum = errorfunc();
    /*"Corrupts" the checksum if errorfunc returns 0 to simulate corrupt checksum*/
    if (corruptSum == 0)
    {
        message.checksum++;
        
    }
    
    /*Sends package if errorfunc returns 1, simulates package lost in sending*/
    if (SendStatus == 1)
    {
        if (sendto(sockfd, &message, sizeof(message), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        {
            printf("Couldn't send message");
        }
    }
/*prints what type of package it was*/
    switch (message.flag)
    {
    case 0:
        if (SendStatus == 1)
        {
            printf("Sending SYN\n");
        }

        else
        {
            printf("SYN Lost\n");
        }
        break;

    case 2:
        if (SendStatus == 1)
        {
            printf("Sending ACK\n");
        }

        else
        {
            printf("ACK Lost\n");
        }
        break;
    case 3:

        if (SendStatus == 1)
        {
            printf("Sending NACK\n");
        }

        else
        {
            printf("NACK Lost\n");
        }
        break;

    case 4:
        if (SendStatus == 1)
        {
            printf("Sent package %d\n", message.seq);
        }
        else
        {
            printf("Package %d lost\n", message.seq);
        }

        break;

    case 5:
        if (SendStatus == 1)
        {
            printf("Sending TEARDOWN\n");
        }
        else
        {
            printf("TEARDOWN LOST\n");
        }

        break;
    }
    if (corruptSum == 0)
    {
        printf("Corrupt CHECKSUM\n");
        
    }
        /*Prints at what time it was recieved*/

    *current = clock();
    double passed = (double)(*current - *start) / CLOCKS_PER_SEC;
    //printf("time elapsed: %f\n\n", passed);
}
/*Creates a socket on the client-side*/
struct sockaddr_in client_setup(struct sockaddr_in serveraddr)
{

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    return serveraddr;
}
/*Function for simulate sliding window moving*/

void slidingwindow()

{
    if (repeatfunc == selectiveRepeat)
    {
        for (int i = windowlow; seqarr[i] != false; i++)
            /*Only moving windowlow if last frame is sent*/
            if (windowhigh == NumOfFrames)
            {
                windowlow++;
            }
            else
            {
                windowlow++;
                windowhigh++;
            }
    }
    
    else
    {
        if (windowhigh == NumOfFrames)
        {
            windowlow++;
        }
        else
        {
            windowlow++;
            windowhigh++;
        }
    }
}
/*Function for creating a checksum that verifies if the package
/*is intact or has been corrupted*/
int checksum(char string[messageLength])
{
    int sumOfNumbers = 0;
    int checksum = 0;
    for (int i = 0; string[i] != '\0'; i++)
    {
        sumOfNumbers = sumOfNumbers + string[i];
    }
    checksum = sumOfNumbers % divisor;
    return checksum;
}
/*Function for simulating timeout*/
rtp TimeOut(rtp message, char Expected[])
{

    /*Stating time*/
    time_t time_start = clock();
    double passed = 0;
    /*Goes trough this loop until timeout*/
    while (passed < timeout_TWH_TEARDOWN)
    {
        /*Check port for recieved message*/
        ioctl(sockfd, FIONREAD, &status);
        if (status > 0)
        {
            message = ReceivFunc(message);
            /*Checks if its the type of package we expected and returns if that is the case*/
            if (strcmp(Expected, message.data) == 0)
            {
                return message;
            }
        }

        /*Ending time and calculating time passed*/
        time_t time_stop = clock();
        passed = (double)(time_stop - time_start) / CLOCKS_PER_SEC;
    }
    if (strcmp(message.data, "ACK") == 0)
    {
        message.flag = NONE;
        return message;
    }
}
/*Function that performs the three-way handshake on the client side*/
void twhclient(rtp message, struct sockaddr_in serveraddr, int sockfd)
{
    int nOfBytes;
    char string[messageLength];
    char Possible[2][10] = {"SYNACK", "NACK"};
    char Expected[10];
    srand(time(NULL));
    message.id = rand();
    message.checksum = 1;
    /*Sends a SYN-package and will move on if it receives a SYNACK from the server
    /*Otherwise it will resend the SYN*/
    while (message.flag != SYNACK || checksum(message.data) != message.checksum)
    {
        int status;
        clientId = message.id;
        message.flag = SYN;
        strcpy(message.data, "SYN");
        strcpy(string, message.data);
        message.checksum = checksum(string);
        message.windowsize = NumOfFrames;
        message.repeatfunc = repeatfunc;
        SendPackage(message);
        strcpy(Expected, Possible[0]);
        message = TimeOut(message, Expected);
    }
    /*Saves the servers id for verifing later packages*/
    serverId = message.id;
    /*Sends a ACK-package and will be done with the three-way handshake if nothing is
    /*returned before timeout, if NACK is received, then it will resend the ACK*/
    while (message.flag != NONE) // && checksum(message.data) == message.checksum
    {

        /*Editing message to return to server*/
        strcpy(message.data, "ACK");
        strcpy(string, message.data);
        message.checksum = checksum(string);
        message.id = clientId;
        message.flag = ACK;
        strcpy(Expected, Possible[1]);
        SendPackage(message);
        message = TimeOut(message, Expected);
    }

    printf("Done with sync\n");
}
/*Function that performs the teardown process from the clients side*/
void teardownClient(rtp message, struct sockaddr_in serveraddr, int sockfd)
{
    OutputStatus = 0;
   

    int nOfBytes;
    char string[messageLength];
    char Possible[2][15] = {"TEARDOWN_ACK", "ACK"};
    char Expected[15];
    /*Sends a teardown-request to the server and will continue if
    /*a Teardown ACK is received otherwise it will resend the request*/
    while (message.flag != TEARDOWN_ACK || checksum(message.data) != message.checksum) // && checksum(message.data) == message.checksum
    {
        int status;
        message.id = clientId;
        message.flag = TEARDOWN;
        strcpy(message.data, "TEARDOWN");
        strcpy(string, message.data);
        message.checksum = checksum(string);
        message.seq = NumOfFrames;
        SendPackage(message);
        strcpy(Expected, Possible[0]);
        message = TimeOut(message, Expected);
    }
    /*Sends a ACK to the client signaling that it has recieved the
    /*Teardown ACK. If nothing is returned before timeout the client will
    /*Be done with the teardown and terminate, otherwises it will resend
    /*the ACK*/
    while (message.flag != NONE) // && checksum(message.data) == message.checksum
    {

        /*Editing message to return to server*/
        strcpy(message.data, "ACK");
        strcpy(string, message.data);
        message.checksum = checksum(string);
        message.id = clientId;
        message.flag = ACK;
        strcpy(Expected, Possible[1]);
        SendPackage(message);
        message = TimeOut(message, Expected);
    }

    printf("Done with teardown\n");
}

void sendingFrames(rtp message)
{
    OutputStatus = 1;
    int nOfBytes, status;
    int sent = windowlow;
        /*List of 10 random words to be used when sending packages*/
    const char *Window[19] = {"This", "Is", "The", "Frames", "That", "We", "Want", "To", "Send", "And","It","Is","Important","That","They","Are","In","Correct","Order"}; //Change to match number of frames
    char string[messageLength];
    int counter = NumOfFrames - 1;
    int test = 1;
    int check = 0;
        /*Setting all positions of seqarr to false, position is switched to true when ACK is recieved*/
    for (int i = 0; i < NumOfFrames; i++)
    {
        seqarr[i] = false;
    }

    do
    {
                /*Sends all available packages in sliding window*/
        while (sent != windowhigh)
        {
            strcpy(message.data, Window[sent]);
            strcpy(string, message.data);
            message.id = clientId;
            message.checksum = checksum(string);
            message.flag = NONE;
            message.seq = sent;

            SendPackage(message);

            sent++;
                        /*Checks if anything is written to socket, and if there is then changes status to positive value*/
            ioctl(sockfd, FIONREAD, &status);
            if (status > 0)
            {
                message = ReceivFunc(message);
                if (message.id == serverId && checksum(message.data) == message.checksum && message.flag == ACK && message.seq == windowlow)
                {
                      /*Indicating ACK recieved*/
                    seqarr[message.seq] = true;

                    slidingwindow();
                      /*To break if last ack is recieved*/
                    if (repeatfunc == goBackN && seqarr[counter] == true)
                    {
                        break;
                    }
                }
                else if (message.id == serverId && checksum(message.data) == message.checksum && message.flag == ACK && repeatfunc == selectiveRepeat)
                {
                    /*Indicating ACK recieved*/
                    seqarr[message.seq] = true;
                }

                else if (message.id == serverId && message.flag == NACK)
                {
                    if (repeatfunc == selectiveRepeat)
                    {
                        message.flag = NONE;
                        message.id = clientId;
                        SendPackage(message);
                    }

                    else
                    {
                        sent = message.seq;
                    }
                }
            }
        }
        time_t time_start = clock();
        double passed = 0;
        test = 1;
        while (passed < timeout)
        {
                /*Checks if anything is written to socket, and if there is then changes status to positive value*/
            ioctl(sockfd, FIONREAD, &status);
            if (status > 0)
            {
                message = ReceivFunc(message);
                if (message.id == serverId && checksum(message.data) == message.checksum && message.seq == windowlow && message.flag == ACK)
                {

                    seqarr[message.seq] = true;
                    slidingwindow();
                    test = 0;
                    break;
                }
                else if (message.id == serverId && checksum(message.data) == message.checksum && message.flag == ACK && repeatfunc == selectiveRepeat)
                {

                    seqarr[message.seq] = true;

                    break;
                }
                else if (message.id == serverId && message.flag == NACK)
                {

                    sent = message.seq;

                    test = 0;
                    break;
                }
            }
            /*Updates value for time passed*/
            time_t time_stop = clock();
            passed = (double)(time_stop - time_start) / CLOCKS_PER_SEC;
                        /*to not go to repeatfunction of all ACKs have been recieved*/
            if (repeatfunc == goBackN && seqarr[counter] == true)
            {
                for (int j = 0; j < NumOfFrames; j++)
                {
                    if(seqarr[j] == false)
                    {
                        check++;
                    }
                }
                if (check == 0)
                {
                    break;
                }
            }
        }
        if (passed > timeout)
        {
            printf("Timeout reached!!\n\n");
            if (repeatfunc == goBackN)
            {
                printf("Go Back N\n");
                sent = windowlow;
                /*Go Back N*/
            }
            else
            {
                printf("Selective Repeat\n\n");
                /*Selective Repeat*/
                message.id = clientId;
                message.flag = NONE;
                int j = windowlow;
                for (j = windowlow; j != windowhigh; j++)
                {
                    printf("%d\n", j);
                    if (seqarr[j] == false)
                    {
                        strcpy(string, message.data);
                        message.checksum = checksum(string);

                        message.seq = j;

                        SendPackage(message);

                        break;
                    }
                }
            }
        }
    } while (windowlow != windowhigh);
    printf("Done with sending packages\n");
}

int main()
{
    start = malloc(sizeof(time_t));
    end = malloc(sizeof(time_t));
    current = malloc(sizeof(time_t));
    /*Starts timer*/
    *start = clock();
    rtp message;
    int nOfBytes;

    serveraddr = client_setup(serveraddr);
    twhclient(message, serveraddr, sockfd);

    sendingFrames(message);

    /*Done with sliding window*/

    teardownClient(message, serveraddr, sockfd);

    /*Prints time program takes*/
    *end = clock();
    double passed = (double)(*end - *start) / CLOCKS_PER_SEC;
    printf("time elapsed: %f\n", passed);

    return 0;
}