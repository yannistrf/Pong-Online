#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"

typedef struct UdpInfo
{
    int recv_fd;
    uint16_t recv_port;
    struct sockaddr_in servaddr;

    int send_fd;
    uint16_t send_port;
    struct sockaddr_in cliaddr;

    UdpInfo()
    {
        recv_fd = -1;
        recv_port = 0;
        servaddr = {};

        send_fd = -1;
        send_port = 0;
        cliaddr = {};
    }

} UdpInfo;


class Server
{
    public:
        Server(uint16_t tcp_port);
        ~Server();
        bool CreateTcpConnectionsWithClients(int *tcp_fd_cl1, int *tcp_fd_cl2);
        bool ExchangeUdpPortsWithClient_CreateUdpSocks(int tcp_fd, int client_no);
    
    private:
        uint16_t tcp_port_;
        UdpInfo client1_info_;
        UdpInfo client2_info_;
};

Server::Server(uint16_t tcp_port)
{
    tcp_port_ = tcp_port;
}

Server::~Server()
{
    close(client1_info_.recv_fd);
    close(client1_info_.send_fd);
    close(client2_info_.recv_fd);
    close(client2_info_.send_fd);
}

/*
    Establishes 2 tcp connections, one for each client and returns the fds
    inside tcp_fd_cl1 and tcp_fd_cl2. Returns false if something goes wrong.
*/
bool Server::CreateTcpConnectionsWithClients(int *tcp_fd_cl1, int *tcp_fd_cl2)
{
    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        print_sock_err("Could not create listen socket: ");
        return false;
    }

    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(tcp_port_); 

    if (bind(listen_fd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) != 0)
    {
        print_sock_err("Failed to bind listen port: ");
        close(listen_fd);
        return false;
    }

    if (listen(listen_fd, 1) != 0)
    {
        print_sock_err("Failed to listen: ");
        close(listen_fd);
        return false;
    }

    printf("Waiting for clients to connect...\n");

    struct sockaddr_in cliaddr = {};
    socklen_t cliaddr_len;
    *tcp_fd_cl1 = accept(listen_fd, (struct sockaddr*) &cliaddr, &cliaddr_len);
    if (*tcp_fd_cl1 == -1)
    {
        print_sock_err("Failed to accept first connection: ");
        close(listen_fd);
        return false;
    }
    printf("First client connected\n");

    struct sockaddr_in cliaddr2 = {};
    socklen_t cliaddr2_len;
    *tcp_fd_cl2 = accept(listen_fd, (struct sockaddr*) &cliaddr2, &cliaddr2_len);
    if (*tcp_fd_cl2 == -1)
    {
        print_sock_err("Failed to accept second connection: ");
        close(listen_fd);
        return false;
    }
    printf("Second client connected\n");

    close(listen_fd);
    return true;
}

/*
    Exchange udp port numbers with the client using the initial tcp
    socket and create udp sockets. Returns false if something goes wrong.
*/
bool Server::ExchangeUdpPortsWithClient_CreateUdpSocks(int tcp_fd, int client_no)
{
    int udp_recv_fd;
    udp_recv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_recv_fd == -1)
    {
        print_sock_err("Could not create udp recv socket: ");
        return false;
    }

    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = 0;

    if (bind(udp_recv_fd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) != 0)
    {
        print_sock_err("Failed to bind udp port: ");
        close(udp_recv_fd);
        return false;
    }

    socklen_t addr_len = sizeof(servaddr);
    getsockname(udp_recv_fd, (struct sockaddr*)&servaddr, &addr_len);
    uint16_t recv_port = ntohs(servaddr.sin_port);
    printf("For client %d, movement will be received at port %u\n", client_no, recv_port);

    // MAYBE DO A CHECK HERE ?
    send(tcp_fd, &recv_port, 2, 0);

    uint16_t send_port;
    recv(tcp_fd, &send_port, 2, 0);
    printf("For client %d, game state will be sent at port %u\n", client_no, send_port);

    close(tcp_fd);

    int udp_send_fd;
    udp_send_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_send_fd == -1)
    {
        print_sock_err("Could not create udp send socket: ");
        return false;
    }

    struct sockaddr_in cliaddr = {};
    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    cliaddr.sin_port = htons(send_port); 

    if (client_no == CLIENT1)
    {
        client1_info_.recv_fd = udp_recv_fd;
        client1_info_.recv_port = recv_port;
        client1_info_.servaddr = servaddr;

        client1_info_.send_fd = udp_send_fd;
        client1_info_.send_port = send_port;
        client1_info_.cliaddr = cliaddr;
    }
    else    /* CLIENT2 */
    {
        client2_info_.recv_fd = udp_recv_fd;
        client2_info_.recv_port = recv_port;
        client2_info_.servaddr = servaddr;

        client2_info_.send_fd = udp_send_fd;
        client2_info_.send_port = send_port;
        client2_info_.cliaddr = cliaddr;
    }

    return true;
}

int main()
{
    Server server(SERVER_LISTEN_PORT);
    
    int tcp_fd_cl1, tcp_fd_cl2;
    server.CreateTcpConnectionsWithClients(&tcp_fd_cl1, &tcp_fd_cl2);
    server.ExchangeUdpPortsWithClient_CreateUdpSocks(tcp_fd_cl1, CLIENT1);
    server.ExchangeUdpPortsWithClient_CreateUdpSocks(tcp_fd_cl2, CLIENT2);
}