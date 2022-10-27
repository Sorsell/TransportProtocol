/*********************************************************************************************************/
/*                                               Server
/*                                                 By
/*                                             Karl Sorsell
/*
/* The server-part of the program. Start this part of the program FIRST and then the client side.
/* Will wait for a client to initate a three-way handshake, exchange the information necessary, and when
/*              the handshake is complete wait to receive packages from the client.
/* The server will remain in the state of receiving frames until receiving a call to teardown and will
/* then move on to the teardown phase. After successfully completing a teardown the server part will
/*              print out the time taken from start to finish and then terminate
/*********************************************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <string.h>  /*for strcmp*/
#include <time.h>    /*for time*/
#include "header.h"
#include <stdbool.h>
#include <sys/ioctl.h>

int nOfFrames, clientId, serverId, sockfd, teardown = 0;
struct sockaddr_in serveraddr, clientaddr;
time_t *start, *end, *current;
#define goBackN 1
#define selectiveRepeat 2
int repeatfunc;
int lastcorrectframe;
float timeout = 0.01;


int OutputStatus = 0;

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
/*Function for creating a socket and binding it on the server side*/
struct sockaddr_in server_setup(struct sockaddr_in serveraddr)
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

    if (bind(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("Socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    return serveraddr;
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

rtp ReceivFunc(rtp message, socklen_t len)
{
    int nOfBytes;

    nOfBytes = recvfrom(sockfd, &message, MAXMSG, MSG_WAITALL, (struct sockaddr *)&clientaddr, &len);
    if (nOfBytes < 0)
    {
        perror("Could not read data from client\n");
        exit(EXIT_FAILURE);
    }
    switch (message.flag)
    {
    case 0:
        if (OutputStatus == 0)
        {
            printf("\nReciving SYN ");
        }
        else
        {
            printf("\nReciving SYN %d", message.seq);
        }
        break;

    case 2:
        if (OutputStatus == 0)
        {
            printf("\nReciving ACK ");
        }
        else
        {
            printf("\nReciving ACK on package %d", message.seq);
        }
        break;

    case 3:
        if (OutputStatus == 0)
        {
            printf("\nReciving NACK ");
        }
        else
        {
            printf("\nReciving NACK package %d", message.seq);
        }
        break;

    case 4:
        if (OutputStatus == 0)
        {
            printf("\nReciving NONE ");
        }
        else
        {
            printf("\nReciving package %d", message.seq);
        }
        break;

    case 5:
        if (OutputStatus == 0)
        {
            printf("\nReciving TEARDOWN\n ");
        }
        else
        {
            printf("\nReciving TEARDOWN\n");
        }
        break;
    }

    *current = clock();
    double passed = (double)(*current - *start) / CLOCKS_PER_SEC;
    //printf("\nTime elapsed: %f\n", passed);

    return message;
}

void SendPackage(rtp message)
{
    int SendStatus = errorfunc();
    int corruptSum = errorfunc();
    if (corruptSum == 0)
    {
        message.checksum++;
    }
    
    if (SendStatus == 1)
    {
        if (sendto(sockfd, &message, sizeof(message), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
        {
            printf("Couldn't send message");
        }
    }
    if (message.flag > 0 && message.flag < 7)
    {
        switch (message.flag)
        {

        case 1:
            if (SendStatus == 1)
            {
                printf("\nSending SYNACK ");
            }

            else
            {
                printf("\nSYNACK Lost ");
            }

            break;

        case 2:
            if (SendStatus == 1)
            {
                printf("\nSending ACK ");
            }

            else
            {
                printf("\nACK Lost ");
            }

            break;
        case 3:

            if (SendStatus == 1)
            {
                printf("\nSending NACK ");
            }

            else
            {
                printf("\nNACK Lost ");
            }

            break;

        case 6:
            if (SendStatus == 1)
            {
                printf("\nSending Teardown ACK ");
            }
            else
            {
                printf("\nTeardown ACK lost ");
            }

            break;
        }

        if (OutputStatus == 1 && message.seq != nOfFrames)
        {
            printf("on package %d", message.seq);
        }
         if(corruptSum == 0){
        printf("\nCorrupt CHECKSUM");
        
    }
        *current = clock();
        double passed = (double)(*current - *start) / CLOCKS_PER_SEC;
        //printf("\nTime elapsed: %f\n", passed);
    }
}
/*Function that performs the three-way handshake on the server side*/
struct sockaddr_in twhserver(rtp message, struct sockaddr_in serveraddr, int sockfd)
{
    srand(time(NULL));
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(struct sockaddr);
    int status;
    char string[messageLength];
    message.flag = NONE;
    message.checksum =1;

    while (message.flag != ACK || checksum(message.data) != message.checksum)
    {
        if (message.flag != SYN && message.flag != NACK)
        {
            message = ReceivFunc(message, len);
        }
        if (message.flag == SYN && checksum(message.data) == message.checksum) //
        {

            /*Saving data from message*/
            nOfFrames = (message.windowsize) + 1;
            clientId = message.id;
            /*Editing message to return to client*/

            message.flag = SYNACK;
            strcpy(message.data, "SYNACK");
            strcpy(string, message.data);
            message.checksum = checksum(string);
            message.id = rand();
            serverId = message.id;
            repeatfunc = message.repeatfunc;

            SendPackage(message);
        }

        /*Stating time*/
        time_t time_start = clock();
        double passed = 0;

        while (passed < timeout)
        {
            /*Check port for recieved message*/
            ioctl(sockfd, FIONREAD, &status);
            if (status > 0)
            {
                /*Reads message from client*/
                message = ReceivFunc(message, len);
                if (message.flag == SYN) // && checksum(message.data) == message.checksum
                {
                    break;
                }
                else if(message.flag == ACK && checksum(message.data) != message.checksum)
                {
                    message.flag = SYNACK;
                    break;
                }
            }
            /*Ending time and calculating time passed*/
            time_t time_stop = clock();
            passed = (double)(time_stop - time_start) / CLOCKS_PER_SEC;
        }

        if (message.flag != SYN && message.flag != ACK )
        {
            strcpy(message.data, "NACK");
            message.flag = NACK;
            SendPackage(message);
        }
    }

    printf("\nDone with sync");
    return clientaddr;
}
/*Function that performs the teardown process from the server side*/
void teardownServer(rtp message, struct sockaddr_in clientaddr, int sockfd)
{
    OutputStatus = 0;
 
    socklen_t len = sizeof(struct sockaddr);
    int status;
    char string[messageLength];
    

    while (message.flag != ACK || checksum(message.data) != message.checksum )
    {
        if (message.flag != TEARDOWN && message.flag != NACK)
        {
            message = ReceivFunc(message, len);
        }
        if (message.flag == TEARDOWN && checksum(message.data) == message.checksum) // && 
        {
            /*Editing message to return to client*/
            message.flag = TEARDOWN_ACK;
            strcpy(message.data, "TEARDOWN_ACK");
            strcpy(string, message.data);
            message.checksum = checksum(string);
            message.id = serverId;
            SendPackage(message);
        }

        /*Stating time*/
        time_t time_start = clock();
        double passed = 0;

        while (passed < timeout)
        {
            /*Check port for recieved message*/
            ioctl(sockfd, FIONREAD, &status);
            if (status > 0)
            {
                /*Reads message from client*/
                message = ReceivFunc(message, len);
                if (message.flag == ACK || message.flag == TEARDOWN) 
                {
                    break;
                }
            }
            /*Ending time and calculating time passed*/
            time_t time_stop = clock();
            passed = (double)(time_stop - time_start) / CLOCKS_PER_SEC;
        }

        if ((message.flag != TEARDOWN && message.flag != ACK) || (message.flag == ACK && message.checksum != checksum(message.data)))
        {

            message.flag = NACK;
            SendPackage(message);
        }
    }

    printf("Done with TEARDOWN\n");
}
void receivingFrames(rtp message)
{
    OutputStatus = 1;
    bool seqarr[nOfFrames - 1];
    for (int o = 0; o < nOfFrames - 1; o++)
    {
        seqarr[o] = false;
    }
    int i = 0;
    int nOfBytes;
    rtp recievedPackage[nOfFrames];
    socklen_t len = sizeof(struct sockaddr);
    while (teardown != 1)
    {
        for (i = 0; seqarr[i] == true; i++)
        {
        }
        recievedPackage[i] = ReceivFunc(recievedPackage[i], len);
        // nOfBytes = recvfrom(sockfd, &recievedPackage[i], MAXMSG, MSG_WAITALL, (struct sockaddr *)&clientaddr, &len);
        // if (nOfBytes < 0)
        // {
        //     perror("Could not read data from client\n");
        //     exit(EXIT_FAILURE);
        // }
       

        if (recievedPackage[i].id == clientId && checksum(recievedPackage[i].data) == recievedPackage[i].checksum && recievedPackage[i].seq == i && recievedPackage[i].flag != TEARDOWN)
        {
            lastcorrectframe = i;
            recievedPackage[i].flag = ACK;
            recievedPackage[i].id = serverId;

            SendPackage(recievedPackage[i]);
            seqarr[recievedPackage[i].seq] = true;
            i++;
        }
        else if (recievedPackage[i].id == clientId && checksum(recievedPackage[i].data) == recievedPackage[i].checksum && recievedPackage[i].flag == TEARDOWN)
        {
            teardown = 1;
        }
        else if (recievedPackage[i].id == clientId && checksum(recievedPackage[i].data) == recievedPackage[i].checksum && (recievedPackage[i].seq < i || recievedPackage[i].seq > i) && repeatfunc == goBackN)
        {
            if (recievedPackage[i].seq < i)
            {
                int t = i;
                i = recievedPackage[i].seq;
                recievedPackage[i] = recievedPackage[t];
                recievedPackage[i].flag = ACK;
                recievedPackage[i].id = serverId;
                // if (sendto(sockfd, &recievedPackage[i], sizeof(recievedPackage), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
                // {
                //     printf("Couldn't send message");
                // }
                // printf("Sent ACK on package %d\n", recievedPackage[i].seq);
                SendPackage(recievedPackage[i]);

                seqarr[recievedPackage[i].seq] = true;

                i++;
            }
            else
            {
                recievedPackage[i].flag = NACK;
                recievedPackage[i].id = serverId;
                recievedPackage[i].seq = i; //

                SendPackage(recievedPackage[i]);
            }
        }
        else if (recievedPackage[i].id == clientId && checksum(recievedPackage[i].data) == recievedPackage[i].checksum && (recievedPackage[i].seq < i || recievedPackage[i].seq > i) && repeatfunc == selectiveRepeat)
        {
            recievedPackage[recievedPackage[i].seq] = recievedPackage[i];
            recievedPackage[recievedPackage[i].seq].id = serverId;
            recievedPackage[recievedPackage[i].seq].flag = ACK;

            SendPackage(recievedPackage[recievedPackage[i].seq]);
            seqarr[recievedPackage[i].seq];
        }
        else
        {
            /*If ID,checksum is incorrect*/
            recievedPackage[i].flag = NACK;
            SendPackage(recievedPackage[i]);
        }
    }
    printf("MESSAGE : ");
    for(int j = 0;j<nOfFrames-1;j++)
    {
        printf(" %s",recievedPackage[j].data);
    }
    teardownServer(recievedPackage[i], clientaddr, sockfd);
}

int main()
{
    /*Starting creation of socket*/
    char buffer[MAXMSG];
    int nOfBytes;
    start = malloc(sizeof(time_t));
    end = malloc(sizeof(time_t));
    current = malloc(sizeof(time_t));

    rtp message;

    /*Finished with creation of socket*/
    serveraddr = server_setup(serveraddr);
    clientaddr = twhserver(message, serveraddr, sockfd);

    receivingFrames(message);

    *end = clock();
    double passed = (double)(*end - *start) / CLOCKS_PER_SEC;
    printf("time elapsed: %f\n", passed);

    /*Teardown*/

    return 0;
}
