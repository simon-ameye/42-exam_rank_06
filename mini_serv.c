#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int sock_arr[65536];
int max_sock = 0;
int next_id = 0;

fd_set readfds;
fd_set writefds;
fd_set active_sock;

char buf_for_write	[42 *4096 + 42];
char buf_for_read	[42 *4096];
char buf_str		[42 *4096];

void fatal_error()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int except)
{
    for(int i = 0; i <= max_sock; i++)
    {
        if(FD_ISSET(i, & writefds) && i != except)
            send(i, buf_for_write, strlen(buf_for_write), 0);
    }
}

int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    int port = atoi(argv[1]);

    bzero(&sock_arr, sizeof(sock_arr));
    FD_ZERO(&active_sock);

    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);

    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = (1 << 24) | 127; //127.0.0.1
	servaddr.sin_port = (port >> 8) | (port << 8);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        fatal_error();

    max_sock = sockfd;
    FD_SET(sockfd, &active_sock);

    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
        fatal_error();
    if(listen(sockfd, 10) < 0)
        fatal_error();

    while(1)
    {
        readfds = writefds = active_sock;

        if(select(max_sock + 1, &readfds, &writefds, NULL, NULL) < 0)
            continue ;
        for(int i = 0; i <= max_sock; i++)
        {
            if(FD_ISSET(i, &readfds) && i == sockfd)
            {
                int fd_client = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                if(fd_client < 0)
                    continue ;
                if(max_sock < fd_client)
                    max_sock = fd_client;
                sock_arr[fd_client] = next_id++;
                FD_SET(fd_client, &active_sock);
                sprintf(buf_for_write, "server: client %d just arrived\n", sock_arr[fd_client]);
                send_all(fd_client);
            }
            if(FD_ISSET(i, &readfds) && i != sockfd)
            {
                int read_res = recv(i, buf_for_read, 42 *4096, 0);
                if(read_res < 0)
                {
                    sprintf(buf_for_write, "server: client %d just arrived\n", sock_arr[i]);
                    send_all(i);
                    FD_CLR(i, &active_sock);
                    close(i);
                    break ;
                }
                else
                {
                    for(int j = 0; j <= read_res; j++)
                    {
                        buf_str[j] = buf_for_read[j];
                        if(buf_str[j] == '\n')
                        {
                            buf_str[j] = '\0';
                            sprintf(buf_for_write, "client %d: %s\n", sock_arr[i], buf_str);
                            send_all(i);
                        }
                    }
                }
            }
        }
    }
}