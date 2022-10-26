//Allowed functions: write, close, select, socket, accept, listen, send, recv, bind, strstr, malloc, realloc, free, calloc, bzero, atoi, sprintf, strlen, exit, strcpy, strcat, memset

#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int maxFd = 0;

int ids				[65536];
int still_typing	[65536];

fd_set ready_to_read;
fd_set ready_to_write;
fd_set active_sockets;

char temp_buff	[4096 * 42];
char clis_buff	[4096 * 42];
char read_buff	[4096 * 42];
char send_buff	[4096 * 42 + 42];

void ft_putstr(char *str)
{
	write(2, str, strlen(str));
	write(2, "\n", 1);
}

void send_to_all(int except)
{
	for(int fd = 0; fd <= maxFd; fd++)
	{
		if(FD_ISSET(fd, &ready_to_write) && except != fd)
			send(fd, send_buff, strlen(send_buff), 0);
	}
}

void fatal_error(void)
{
	ft_putstr("Fatal error");
	exit(1);
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		ft_putstr("Wrong number of arguments");
		exit(1);
	}

	int client_index = 0;

	uint16_t port = atoi(argv[1]);
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = (1 << 24) | 127;
	serveraddr.sin_port = (port >> 8) | (port << 8);
	socklen_t serveraddr_len = sizeof(serveraddr);

	int master_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(master_sock == -1)
		fatal_error();

	if(bind(master_sock, (struct sockaddr*)&serveraddr, serveraddr_len) == -1)
	{
		close(master_sock);
		fatal_error();
	}

	if(listen(master_sock, 128) == -1)
	{
		close(master_sock);
		fatal_error();
	}

	bzero(ids, sizeof(ids));
	FD_ZERO(&active_sockets);
	FD_SET(master_sock, &active_sockets);
	maxFd = master_sock;

	while(1)
	{
		ready_to_write = active_sockets;
		ready_to_read = active_sockets;
		if(select(maxFd + 1, &ready_to_read, &ready_to_write, 0, 0) <= 0)
			continue;

		for(int fd = 0; fd <= maxFd; fd++)
		{
			if(FD_ISSET(fd, &ready_to_read))
			{
				if(fd == master_sock)
				{
					int client_sock = accept(fd, (struct sockaddr*)&serveraddr, &serveraddr_len);
					if(client_sock == -1)
						continue;

					FD_SET(client_sock, &active_sockets);
					ids[client_sock] = client_index++;

					still_typing[client_sock] = 0;
					if(maxFd < client_sock)
						maxFd = client_sock;

					sprintf(send_buff, "server: client %d just arrived\n", ids[client_sock]);
					send_to_all(fd);
					break;
				}
				else
				{
					int recv_size = recv(fd, read_buff, 4096 * 42, 0);
					if(recv_size <= 0)
					{
						sprintf(send_buff, "server: client %d just left\n", ids[fd]);
						send_to_all(fd);
						FD_CLR(fd, &active_sockets);
						close(fd);
						break;
					}
					else
					{
						/*
						for(int i = 0, j = 0; i < recv_size; i++, j++)
						{
							temp_buff[j] = read_buff[i];
							if(temp_buff[j] == '\n')
							{
								temp_buff[j + 1] = '\0';
								if(still_typing[fd])
									sprintf(send_buff, "%s", temp_buff);
								else
									sprintf(send_buff, "client %d: %s", ids[fd], temp_buff);
								still_typing[fd] = 0;
								send_to_all(fd);
								j = -1;
							}
							else if(i == (recv_size - 1))
							{
								temp_buff[j + 1] = '\0';
								if (still_typing[fd])
									sprintf(send_buff, "%s", temp_buff);
								else
									sprintf(send_buff, "client %d: %s", ids[fd], temp_buff);
								still_typing[fd] = 1;
								send_to_all(fd);
								break;
							}
						}
						*/
						read_buff[recv_size] = '\0';
						temp_buff[0] = '\0';
						send_buff[0] = '\0';

						sprintf(clis_buff, "client %d: ", ids[fd]);

						//if (!still_typing[fd])
						//{
						//	strcat(temp_buff, clis_buff);
						//	still_typing[fd] = 1;
						//}

						//bzero(temp_buff, sizeof(temp_buff));
						char *ret = read_buff;
						while (ret[0])
						{
							//strcat(send_buff, clis_buff);
							strcpy(temp_buff, ret);

							int i = 0;
							while (temp_buff[i])
							{
								if (temp_buff[i] == '\n')
								{
									temp_buff[i + 1] = 0;
									ret += i + 1;
									strcat(send_buff, clis_buff);
									strcat(send_buff, temp_buff);
									break;
								}
								printf("i : %d\n", i);

								i++;
							}
						}
						//strcat(temp_buff, read_buff);
						//strcpy(send_buff, temp_buff);
						send_to_all(fd);
					}
				}
			}
		}
	}
	return(0);
}