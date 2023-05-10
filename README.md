This program is a terminal program that simulates a reliable and efficient transport protocol, that could be used on top of UDP.
The program consist of two parts, a server and a client. 
The server acts as the reciever of packages.
The client acts as the sender.

The program is written in C using using a laptop using ubuntu linux.

After building the project it creates 2 executable files, one for the server and one for the client.

By running these simultaneously in separate terminal windows you can see all outputs and timestamps.
The server and client will perform a three-way handshake and establish a connection. After the connection is made the client will send a set of random packages in the form of structs containing the checksum, id, sequence number, flag, window size, text string and repeat function.
When all packets have been sent and all ACKS received the client calls for teardown, the connection is then terminated.
