/* Eric Sobel
 * esobel@calpoly.edu
 */
 
struct __attribute__ ((__packed__)) header {
   uint32_t seq;
   uint8_t flag;
};

struct link {
   int socket;
   char handle[101];
};

int init_server(short sckt);
int add_client(int server_socket);
int check_sockets(struct link *links, int connections);
void add_to_list(int new, struct link **links, int *connections, int *max);
char* setup_header(char *pkt, int flag);
void add_handle(int socket, char *handle, struct link *links, int connections);
