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

int max_fd = 0;

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
	for(int fd = 0; fd <= max_fd; fd++)
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
	max_fd = master_sock;

	while(1)
	{
		ready_to_write =	active_sockets;
		ready_to_read =		active_sockets;
		if(select(max_fd + 1, &ready_to_read, &ready_to_write, 0, 0) <= 0)
			continue;

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &ready_to_read))
			{
				if (fd == master_sock)
				{
					int client_sock = accept(fd, (struct sockaddr*)&serveraddr, &serveraddr_len);
					if (client_sock == -1)
						continue;

					FD_SET(client_sock, &active_sockets);
					ids[client_sock] = client_index++;

					still_typing[client_sock] = 0;

					if(max_fd < client_sock)
						max_fd = client_sock;

					sprintf(send_buff, "server: client %d just arrived\n", ids[client_sock]);
					send_to_all(fd);
					break;
				}
				else
				{
					int recv_size = recv(fd, read_buff, 4096 * 42, 0);
					if (recv_size <= 0)
					{
						sprintf(send_buff, "server: client %d just left\n", ids[fd]);
						send_to_all(fd);
						FD_CLR(fd, &active_sockets);
						close(fd);
						break;
					}
					else
					{
						read_buff[recv_size] =	'\0';
						temp_buff[0] =			'\0';
						send_buff[0] =			'\0';

						sprintf(clis_buff, "client %d: ", ids[fd]);

						char *ret = read_buff; //reticule on read buffer. Will move from sentences to sentences
						while (ret[0])
						{
							strcpy(temp_buff, ret); //we work on a copy of ret

							if (!still_typing[fd]) //add "client %d:" if it is a new sentence
								strcat(send_buff, clis_buff);

							int i = 0;
							while (1)
							{
								if (temp_buff[i] == '\n') //end of sentence :
								{
									temp_buff[i + 1] = 0; //close temp_buff
									ret += i + 1; //move ret to new sentence
									strcat(send_buff, temp_buff); //add temp_buff to send_buff
									still_typing[fd] = 0; //end of sentence
									break;
								}
								if (!temp_buff[i]) //partial end, sentence not finished :
								{
									ret += i; //move ret to the end of buffer
									strcat(send_buff, temp_buff); //add the partial sentence to buff
									still_typing[fd] = 1; //not the end of a sentence
									break;
								}
								i++;
							}
						}
						send_to_all(fd); //send buff
					}
				}
			}
		}
	}
	return(0);
}