/* Eric Sobel
 * esobel@calpoly.edu
 */

struct __attribute__ ((__packed__)) header {
   uint32_t seq;
   uint8_t flag;
};

int client_setup(char *host_name, char *port);
char* setup_header(char *pkt, int flag);
int check_select(int socket);
