#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define SERVER_LISTEN_PORT 8080
#define print_sock_err(str)   perror(str); perror(strerror(errno)); perror("\n");

typedef struct
{
    int recv_fd;
    uint16_t recv_port;
    struct sockaddr_in cliaddr;

    int send_fd;
    uint16_t send_port;
    struct sockaddr_in servaddr;
} UdpInfo;

class Client
{
    public:
        Client();
        ~Client();
        bool CreateTcpConnectionWithServer(int *tcp_fd);
        bool ExchangeUdpPortsWithServer_CreateUdpSocks(int tcp_fd);
    
    private:
        UdpInfo server_info_;
};

Client::Client()
{

}

Client::~Client()
{
    close(server_info_.recv_fd);
    close(server_info_.send_fd);
}

/*
    Establishes tcp connection with server.
    Returns false if something goes wrong.
*/
bool Client::CreateTcpConnectionWithServer(int *tcp_fd)
{
    *tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*tcp_fd == -1)
    {
        print_sock_err("Could not create tcp socket: ");
        return false;
    }

    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(SERVER_LISTEN_PORT);

    if (connect(*tcp_fd, (const sockaddr*) &servaddr, sizeof(servaddr)) == -1)
    {
        print_sock_err("Could not connect to server: ");
        return false;
    }
    printf("Connected to server\n");

    return true;
}

/*
    Exchange udp port numbers with the server using the initial tcp
    socket and create udp sockets. Returns false if something goes wrong.
*/
bool Client::ExchangeUdpPortsWithServer_CreateUdpSocks(int tcp_fd)
{
    uint16_t send_port;
    recv(tcp_fd, &send_port, 2, 0);
    printf("Will be sending movement to client at port %u\n", send_port);

    int udp_send_fd;
    udp_send_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_send_fd == -1)
    {
        print_sock_err("Could not create udp send socket: ");
        return false;
    }

    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(send_port);

    int udp_recv_fd;
    udp_recv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_recv_fd == -1)
    {
        print_sock_err("Could not create udp recv socket: ");
        return false;
    }

    struct sockaddr_in cliaddr = {};
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    cliaddr.sin_port = 0;

    if (bind(udp_recv_fd, (const struct sockaddr*) &cliaddr, sizeof(cliaddr)) != 0)
    {
        print_sock_err("Failed to bind udp port: ");
        close(udp_recv_fd);
        return false;
    }

    socklen_t addr_len = sizeof(cliaddr);
    getsockname(udp_recv_fd, (struct sockaddr*)&cliaddr, &addr_len);
    uint16_t recv_port = ntohs(cliaddr.sin_port);
    printf("Game state will be received at port %u\n", recv_port);

    // MAYBE DO A CHECK HERE ?
    send(tcp_fd, &recv_port, 2, 0);

    close(tcp_fd);

    server_info_.recv_fd = udp_recv_fd;
    server_info_.recv_port = recv_port;
    server_info_.cliaddr = cliaddr;

    server_info_.send_fd = udp_send_fd;
    server_info_.send_port = send_port;
    server_info_.servaddr = servaddr;

    return true;
}

int main()
{
    Client client;

    int tcp_fd;
    client.CreateTcpConnectionWithServer(&tcp_fd);
    client.ExchangeUdpPortsWithServer_CreateUdpSocks(tcp_fd);
}