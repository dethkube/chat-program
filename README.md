# Chat Program
# Text Chat server and client programs using TCP
# Author: Eric Sobel
# Credits: networks.h and Makefile supplied by Prof. Hugh Smith for CPE 464

These two programs fuction together as a text-based chat service. Each instance of the chat client
connects to the server using either a random or specified port, and is then able to send and receive
messages to other users through the server.

Available commands for clients are as follows:

%M <user> <message> To send a message to a given user

%B <message> To broadcast a message to all online users

%L To request a list of users currently online

%E To safely disconnect and exit the client


To compile:

$ make                  (for both)
-or-
$ make server/cclient   (for one or the other)


To run server:

$ ./server [optional-port-number]

which prints the port number used (either random or specified by the user) and runs continuously


To run the client:

$ ./cclient <username> <server-name/address> <server-port>

if the connection is successful, use the command above to talk to other clients
