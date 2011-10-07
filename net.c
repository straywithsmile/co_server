#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>

int open_service(unsigned short port)
{
	int rv;
	int optval;
	struct sockaddr_in sin;
	socklen_t sin_len;
	int flags;
	
	int server_port;

	if ((server_port = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return -1;
	}
	if (setsockopt(server_port, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
	{
		return -2;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons((u_short) port);

	if (bind(server_port, (struct sockaddr *)&sin, sizeof(sin)) == -1)
	{
		return -3;
	}
	sin_len = sizeof(sin);
	if (getsockname(server_port, (struct sockaddr *)&sin, &sin_len) == -1)
	{
		return -4;
	}

	flags = fcntl(server_port, F_GETFL, 0);
	if (fcntl(server_port, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		return -5;
	}

	if (listen(server_port, 50) == -1)
	{
		return -6;
	}
	return server_port;
}

int connect_service(const char *host_ip, const unsigned int port)
{
	int fd;
	int rv;
	int flags;
	int optval;

	struct sockaddr_in dest_addr;
	int client_port;

	client_port = socket(AF_INET, SOCK_STREAM, 0);
	if (client_port == -1)
	{
		return -1;
	}
	if (setsockopt(client_port, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
	{
		return -2;
	}

	flags = fcntl(client_port, F_GETFL, 0);

	if (fcntl(client_port, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		return -3;
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons((u_short) port);
	dest_addr.sin_addr.s_addr = inet_addr(host_ip);

	if (connect(client_port, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
	{
		if (errno != EINPROGRESS)
		{
			perror(strerror(errno));
			fprintf(stderr, "connect failed %d\n", rv, errno);
			return -4;
		}
	}
	return client_port;
}

unsigned short get_main_port()
{
	return 30058;
}
