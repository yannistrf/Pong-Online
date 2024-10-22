#define SERVER_LISTEN_PORT 8080

#define print_sock_err(str)   perror(str); perror(strerror(errno)); perror("\n");

#define CLIENT1 1
#define CLIENT2 2