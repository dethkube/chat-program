# Makefile for CPE464 program 1

CC= gcc
CFLAGS= -g

# The  -lsocket -lnsl are needed for the sockets.
# The -L/usr/ucblib -lucb gives location for the Berkeley library needed for
# the bcopy, bzero, and bcmp.  The -R/usr/ucblib tells where to load
# the runtime library.

# The next line is needed on Sun boxes (so uncomment it if your on a
# sun box)
# LIBS =  -lsocket -lnsl

# For Linux/Mac boxes uncomment the next line - the socket and nsl
# libraries are already in the link path.
LIBS =

all:   cclient server

cclient: cclient.c
	$(CC) $(CFLAGS) -o cclient cclient.c $(LIBS)

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

clean:
	rm server cclient



