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

char temp_buff		[4096 * 42];
char read_buff		[4096 * 42];
char send_buff		[4096 * 42 + 42];

fd_set ready_to_read;
fd_set ready_to_write;
fd_set active_sockets;

void ft_putstr(char *str)
{
	write(2, str, strlen(str));
	write(2, "\n", 1);
}

void fatal_error(void)
{
	ft_putstr("Fatal error");
	exit(1);
}

void flush_send_buff(int sender_fd) //send send_buff to everyone except sender
{
	for(int fd = 0; fd <= max_fd; fd++)
	{
		if(FD_ISSET(fd, &ready_to_write) && sender_fd != fd)
			send(fd, send_buff, strlen(send_buff), 0);
	}
}

void flush_temp_buff(int sender_fd) //send temp_buff to everyone except sender and eventually adds "client %d: "
{
	if (still_typing[sender_fd])
		sprintf(send_buff, "%s", temp_buff);
	else
		sprintf(send_buff, "client %d: %s", ids[sender_fd], temp_buff);
	flush_send_buff(sender_fd);
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
					flush_send_buff(fd);
					break;
				}
				else
				{
					int recv_size = recv(fd, read_buff, 4096 * 42, 0);
					if (recv_size <= 0)
					{
						sprintf(send_buff, "server: client %d just left\n", ids[fd]);
						flush_send_buff(fd);
						FD_CLR(fd, &active_sockets);
						close(fd);
						break;
					}
					else
					{
						read_buff[recv_size] =	'\0';								//make read_buff null terminated to iterate over it easily
						temp_buff[0] =			'\0';								//reset temp_buff

						char *ret = read_buff;										//reticule on read buffer. Will move from sentences to sentences
						while (ret[0] != '\0')
						{
							strcpy(temp_buff, ret);									//we work on a copy of read_buff, at position ret

							int i = 0;
							while (temp_buff[i] != '\0' && temp_buff[i] != '\n')	//i will stop on a \n or \0
								i++;

							if (temp_buff[i] == '\0')								//partial end, no more to read
							{
								flush_temp_buff(fd);
								still_typing[fd] = 1;								//not the end of a sentence
								break;
							}
							if (temp_buff[i] == '\n')								//end of sentence, but maybe there is still more to read
							{
								temp_buff[i + 1] = '\0';							//close temp_buff
								ret += i + 1;										//ret jump to begining of new sentence
								flush_temp_buff(fd);
								still_typing[fd] = 0;								//end of sentence
							}
						}
					}
				}
			}
		}
	}
	return(0);
}