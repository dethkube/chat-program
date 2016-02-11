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

#include "cclient.h"


int main(int argc, char *argv[]) {
   int socket_num;
   char *buf, *p;
   unsigned char c;
   int bufsize = 0;
   int send_len = 0;
   int sent = 0;
   int handle_len;
   int select;
   int message_len;
   int handles, curr;
   uint32_t *tmp;
   struct header *hdr;


   /* check command line arguments  */
   if(argc != 4)
   {
      printf("Usage: %s handle server-name server-port\n", argv[0]);
      exit(1);
   }

   handle_len = strlen(argv[1]);
   if (handle_len > 100) {
      printf("Handle too long, 100 char max\n");
      exit(1);
   }

   /* set up the socket for TCP transmission  */
   socket_num = client_setup(argv[2], argv[3]);

   /* initialize data buffer for the packet */
   bufsize = 1224;
   buf = (char *) malloc(bufsize);

   p = setup_header(buf, 1);

   *p++ = handle_len;
   strncpy(p, argv[1], handle_len);
   p[handle_len++] = 0;
   send_len = 6 + handle_len;

   sent = send(socket_num, buf, send_len, 0);
   if(sent < 0)
   {
      perror("send call");
      exit(-1);
   }

   //get confirmation of good handle
   if ((message_len = recv(socket_num, buf, bufsize, 0)) < 0)
   {
      perror("recv call");
      exit(-1);
   }
   hdr = (struct header*)buf;
   if (hdr->flag == 3) {
      printf("Error on initial packet (handle already exists)\n");
      exit(1);
   }
   else if (hdr->flag != 2) {
      printf("Error on initial packet (Unknown)\n");
      exit(1);
   }

   printf("> ");
   fflush(stdout);

   while (1) {
      while ((select = check_select(socket_num)) < 0);
      // handles input on stdin
      if (!select) {
         if (getchar() == '%') {
            c = getchar();
            // M for message
            if (c == 'M') {
               p = setup_header(buf, 6) + 1;
               while ((c = getchar()) == ' ');
               ungetc(c, stdin);
               c = 0;
               while ((p[c] = getchar()) != ' ' && p[c] != '\n' && c < 100)
                  c++;
               ungetc(p[c], stdin);
               *(p - 1) = c;
               p += c;
               if (c == 100) {
                  c = 0;
                  while (c != ' ' && c != '\n')
                     c = getchar();
                  ungetc(c, stdin);
                  printf("Handle too long, 100 char limit\n");
               }

               *p++ = strlen(argv[1]);
               memcpy(p, argv[1], strlen(argv[1]));
               p += strlen(argv[1]);

               c = 0;
               while (c != ' ' && c != '\n')
                  c = getchar();
               ungetc(c, stdin);

               send_len = 0;
               if (c != '\n') {
                  getchar();
                  while ((p[send_len] = getchar()) != '\n' && send_len < 1000)
                     send_len++;

                  if (send_len == 1000) {
                     while (getchar() != '\n');
                     printf("Message too long, 1000 char limit\n");
                  }
               }
               p[send_len++] = '\0';
               send_len += 7 + c + strlen(argv[1]);
            }
            // B for broadcast
            else if (c == 'B') {
               p = setup_header(buf, 5);
               *p++ = strlen(argv[1]);
               memcpy(p, argv[1], strlen(argv[1]));
               p += strlen(argv[1]);
               send_len = 0;
               getchar();
               while ((p[send_len] = getchar()) != '\n' && send_len < 1000)
                  send_len++;

               if (send_len == 1000) {
                  while (getchar() != '\n');
                  printf("Message too long, 1000 char limit\n");
               }

               p[send_len++] = '\0';
               send_len += 7 + strlen(argv[1]);
            }
            // L for request handles
            else if (c == 'L') {
               setup_header(buf, 10);
               send_len = 5;
            }
            // E for exit
            else if (c == 'E') {
               setup_header(buf, 8);
               send_len = 5;
            }
            else {
               printf("Error: Invalid Command\n");
            }
         
            /* now send the data */
            sent = send(socket_num, buf, send_len, 0);
            if(sent < 0)
            {
               perror("send call");
               exit(-1);
            }

            if (c != 'L' && c != 'E') {
               printf("> ");
               fflush(stdout);
            }
         }
      }
      // handles input from the server
      else {
         if ((message_len = recv(socket_num, buf, bufsize, 0)) < 0)
         {
            perror("recv call");
            exit(-1);
         }
         if (!message_len) {
            printf("Error: Lost connection to server\n");
            exit(1);
         }
         hdr = (struct header*)buf;
         // flag 6 => M for message
         if (hdr->flag == 6) {
            p = buf + 5;
            handle_len = *p++;
            p += handle_len;
            handle_len = *p++;
            printf("\n");
            while (handle_len--)
               putchar(*p++);
            printf(": ");
            printf("%s\n", p);
            printf("> ");
            fflush(stdout);
         }
         // flag 5 => B for broadcast
         else if (hdr->flag == 5) {
            p = buf + 5;
            handle_len = *p++;
            printf("\n");
            while (handle_len--)
               putchar(*p++);
            printf(": ");
            printf("%s\n", p);
            printf("> ");
            fflush(stdout);
         }
         // flag 7 => error, bad dest
         else if (hdr->flag == 7) {
            p = buf + 5;
            p[*p++] = 0;
            printf("Client with handle %s does not exist.\n", p);
            printf("> ");
            fflush(stdout);
         }
         // flag 9 => ack from exit
         else if (hdr->flag == 9) {
            break;
         }
         // flag 11 => number of handles
         else if (hdr->flag == 11) {
            p = buf + 5;
            handles = *(uint32_t*)p;
            curr = 1;
            p = setup_header(buf, 12);
            tmp = (uint32_t*)p;
            *tmp = curr;
            sent = send(socket_num, buf, 9, 0);
            if(sent < 0)
            {
               perror("send call");
               exit(-1);
            }
         }
         // flag 13 => actual handle
         else if (hdr->flag == 13) {
            p = buf + 5;
            p[*p++ + 1] = 0;
            printf("%s\n", p);
            curr++;
            if (curr <= handles) {
               p = setup_header(buf, 12);
               tmp = (uint32_t*)p;
               *tmp = curr;
               sent = send(socket_num, buf, 9, 0);
               if(sent < 0)
               {
                  perror("send call");
                  exit(-1);
               }
            }
            else {
               printf("> ");
               fflush(stdout);
            }
         }
         // flag 14 => no handle
         else if (hdr->flag == 14) {
            curr++;
            if (curr <= handles) {
               p = setup_header(buf, 12);
               tmp = (uint32_t*)p;
               *tmp = curr;
               sent = send(socket_num, buf, 9, 0);
               if(sent < 0)
               {
                  perror("send call");
                  exit(-1);
               }
            }
            else {
               printf("> ");
               fflush(stdout);
            }
         }
      }
   }

   close(socket_num);

   return 0;
}

int client_setup(char *host_name, char *port)
{
   int socket_num;
   struct sockaddr_in remote;
   struct hostent *hp;

   // create the socket
   if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("socket call");
      exit(-1);
   }
   
   // designate the addressing family
   remote.sin_family = AF_INET;

   // get the address of the remote host and store
   if ((hp = gethostbyname(host_name)) == NULL)
   {
     printf("Error getting hostname: %s\n", host_name);
     exit(-1);
   }
   
   memcpy((char*)&remote.sin_addr, (char*)hp->h_addr, hp->h_length);

   // get the port used on the remote side and store
   remote.sin_port = htons(atoi(port));

   if(connect(socket_num, (struct sockaddr*)&remote, sizeof(struct sockaddr_in)) < 0)
   {
      perror("connect call");
      exit(-1);
   }

   return socket_num;
}

char* setup_header(char *pkt, int flag) {
   static uint32_t count = 1;
   struct header *hdr = (struct header*)pkt;

   hdr->seq = htonl(count++);
   hdr->flag = flag;

   return pkt + 5;
}

int check_select(int socket) {
   fd_set rd_sckts;
   struct timeval t;
   int ret, i, num = 0;

   FD_ZERO(&rd_sckts);
   FD_SET(0, &rd_sckts);
   FD_SET(socket, &rd_sckts);
   num = socket + 1;

   t.tv_sec = 0;
   t.tv_usec = 0;

   ret = select(num, &rd_sckts, NULL, NULL, &t);

   if (ret < 0) {
      perror("select call");
      exit(-1);
   }
   else if (ret) {
      if (FD_ISSET(0, &rd_sckts)) {
         return 0;
      }
      else if (FD_ISSET(socket, &rd_sckts)) {
         return 1;
      }
      else {
         printf("Input on wrong stream?");
         exit(1);
      }
   }
   
   return -1;
}
