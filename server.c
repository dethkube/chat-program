/* Eric Sobel
 * esobel@calpoly.edu
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.h"


int main(int argc, char const *argv[]) {
   int server_socket = 0;
   char *buf, *p;
   int buffer_size = 1224;
   int message_len = 0;
   int sckt = 0;
   struct link *links;
   int connections = 0;
   int max = 20;
   int new = 0;
   int i, sent;
   uint32_t *tmp;
   char handle[101];
   struct header *hdr;

   //initial max links is 20
   links = (struct link*)calloc(sizeof(struct link), max);
   
   //create packet buffer
   buf =  (char *) malloc(buffer_size);

   //create the server socket
   if (argc == 2) {
      server_socket = init_server(atoi(argv[1]));
   }
   else {
      server_socket = init_server(0);
   }

   //add to list of sockets
   links[0].socket = server_socket;
   connections++;

   //listen on server socket
   if (listen(server_socket, 10) < 0)
   {
     perror("listen call");
     exit(-1);
   }

   while (1) {
      while ((sckt = check_sockets(links, connections)) < 0);
      // main socket for new connections
      if (!sckt) {
         new = add_client(server_socket);
         add_to_list(new, &links, &connections, &max);
      }
      // client socket handling
      else {
         //now get the data on the client_socket
         if ((message_len = recv(links[sckt].socket, buf, buffer_size, 0)) < 0)
         {
            perror("recv call");
            exit(-1);
         }
         // connection failure
         if (!message_len) {
            links[sckt].socket = 0;
            links[sckt].handle[0] = 0;
         }
         // type specific handling
         else {
            hdr = (struct header*)buf;
            // add handle request
            if (hdr->flag == 1) {
               add_handle(sckt, buf + 5, links, connections);
            }
            // M for message
            else if (hdr->flag == 6) {
               p = buf + 5;
               strncpy(handle, p + 1, *p);
               handle[*p] = '\0';
               p += *p + 1;
               if ((i = handle2socket(handle, links, connections)) < 0) {
                  p = setup_header(buf, 7);
                  message_len = 6 + *p;
                  sent = send(links[sckt].socket, buf, message_len, 0);
                  if(sent < 0)
                  {
                     perror("send call");
                     exit(-1);
                  }
               }
               else {
                  sent = send(links[i].socket, buf, message_len, 0);
                  if(sent < 0)
                  {
                     perror("send call");
                     exit(-1);
                  }
               }
            }
            // B for broadcast
            else if (hdr->flag == 5) {
               for (i = 1; i < connections; i++) {
                  if (links[i].socket && i != sckt) {
                     sent = send(links[i].socket, buf, message_len, 0);
                     if(sent < 0)
                     {
                        perror("send call");
                        exit(-1);
                     }
                  }
               }
            }
            // client terminating
            else if (hdr->flag == 8) {
               setup_header(buf, 9);
               sent = send(links[sckt].socket, buf, message_len, 0);
               if(sent < 0)
               {
                  perror("send call");
                  exit(-1);
               }
               links[sckt].socket = 0;
               links[sckt].handle[0] = 0;
            }
            // request handles
            else if (hdr->flag == 10) {
               p = setup_header(buf, 11);
               tmp = (uint32_t*)p;
               *tmp = connections - 1;
               sent = send(links[sckt].socket, buf, 9, 0);
               if(sent < 0)
               {
                  perror("send call");
                  exit(-1);
               }
            }
            // return a handle
            else if (hdr->flag == 12) {
               tmp = (uint32_t*)(buf + 5);
               i = *tmp;
               if (links[i].socket) {
                  p = setup_header(buf, 13);
                  *p = strlen(links[i].handle);
                  strncpy(p + 1, links[i].handle, *p);
                  message_len = 6 + *p;
               }
               else {
                  setup_header(buf, 14);
                  message_len = 5;
               }
               sent = send(links[sckt].socket, buf, message_len, 0);
               if(sent < 0)
               {
                  perror("send call");
                  exit(-1);
               }
            }
         }
      }
   }

   return 0;
}

int init_server(short sckt) {
   int server_socket = 0;
   struct sockaddr_in local;      /* socket address for local side  */
   socklen_t len = sizeof(local);  /* length of local address        */

   /* create the socket  */
   server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if(server_socket < 0)
   {
      perror("socket call");
      exit(1);
   }

   local.sin_family = AF_INET;         //internet family
   local.sin_addr.s_addr = INADDR_ANY; //wild card machine address
   
   // is either zero or a given socket
   local.sin_port = htons(sckt);

   /* bind the name (address) to a port */
   if (bind(server_socket, (struct sockaddr *) &local, sizeof(local)) < 0)
   {
      perror("bind call");
      exit(-1);
   }
   
   //get the port name and print it out
   if (getsockname(server_socket, (struct sockaddr*)&local, &len) < 0)
   {
      perror("getsockname call");
      exit(-1);
   }

   printf("Server is using port %hu\n", ntohs(local.sin_port));
         
   return server_socket;
}

int add_client(int server_socket) {
   int client_socket = 0;
  
   if ((client_socket = accept(server_socket, (struct sockaddr*)0, (socklen_t *)0)) < 0)
   {
      perror("accept call");
      exit(-1);
   }
  
   return client_socket;
}

int check_sockets(struct link *links, int connections) {
   fd_set rd_sckts;
   struct timeval t;
   int ret, i, num = 0;

   FD_ZERO(&rd_sckts);
   for (i = 0; i < connections; i++) {
      if (links[i].socket) {
         FD_SET(links[i].socket, &rd_sckts);
         if (links[i].socket > num) {
            num = links[i].socket;
         }
      }
   }
   num++;

   t.tv_sec = 0;
   t.tv_usec = 0;

   ret = select(num, &rd_sckts, NULL, NULL, &t);

   if (ret < 0) {
      perror("select call");
      exit(-1);
   }
   else if (ret) {
      for (i = 0; i < connections; i++) {
         if (FD_ISSET(links[i].socket, &rd_sckts)) {
            return i;
         }
      }
   }

   return -1;
}

void add_to_list(int new, struct link **links, int *connections, int *max) {
   int i;
   void *tmp;

   //update max connections if needed
   if (*connections == *max) {
      tmp = calloc(sizeof(struct link), *max * 2);
      memcpy(tmp, (void*)*links,  *max * sizeof(struct link));
      *max *= 2;
      *links = (struct link*)tmp;
   }

   //tries to fill an empty connection slot
   for (i = 0; i < *connections; i++) {
      if (!(*links)[i].socket) {
         (*links)[i].socket = new;
         return;
      }
   }

   //if all slots full, fill next and inc num slots
   (*links)[*connections].socket = new;
   (*connections)++;
   return;
}

char* setup_header(char *pkt, int flag) {
   static uint32_t count = 1;
   struct header *hdr = (struct header*)pkt;

   hdr->seq = htonl(count++);
   hdr->flag = flag;

   return pkt + 5;
}

void add_handle(int socket, char *handle, struct link *links, int connections) {
   static struct header hdr;
   int len = *handle++;
   int i, sent;
   for (i = 1; i < connections; i++) {
      if (!strcmp(links[i].handle, handle)) {
         setup_header((char*)&hdr, 3);
         sent = send(links[socket].socket, (char*)&hdr, 5, 0);
         if(sent < 0)
         {
            perror("send call");
            exit(-1);
         }
      }
   }
   strncpy(links[socket].handle, handle, len);
   links[socket].handle[len] = '\0';
   setup_header((char*)&hdr, 2);
   sent = send(links[socket].socket, (char*)&hdr, 5, 0);
   if(sent < 0)
   {
      perror("send call");
      exit(-1);
   }
}

int handle2socket(char *handle, struct link *links, int connections) {
   int i;
   for (i = 1; i < connections; i++) {
      if (!strcmp(handle, links[i].handle)) {
         return i;
      }
   }
   return -1;
}
