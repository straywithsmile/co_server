#include "protocol.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

static struct kevent main_event[256];
static int main_kq;

void log_and_quit(int code, char *str)
{
	char debug[10240];
	serial_string(debug, "exit with code %d %s\n", code, str);
	log_string("server.log", debug);
	exit(code);
}

bool init_all_module()
{
	init_gs_module();
}

bool init_main_event()
{
	main_kq = kqueue();
	if (main_kq == -1)
	{
		log_and_quit(2, "can't initial kqueue");
	}
	return true;
}

void add_read_event(int kq, int fd, int backlog, long type)
{
	struct kevent ev;
	int result;
	EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, backlog, (void *)type);
	if ((result = kevent(kq, &ev, 1, NULL, 0, NULL)) < 0)
	{
		fprintf(stderr, "add_read_event fail %d %d\n", result, errno);
	}
}

void remove_read_event(int kq, int fd)
{
	struct kevent ev;
	EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	if (kevent(kq, &ev, 1, NULL, 0, NULL) < 0)
	{
		fprintf(stderr, "add_read_event fail");
	}
}

void remove_read_event_from_main_kqueue(int fd)
{
	remove_read_event(main_kq, fd);
}

bool init_main_socket()
{
	int fd = open_service(get_main_port());

	if (fd < 0)
	{
		log_and_quit(1, "can't open_service");
	}

	add_read_event(main_kq, fd, 50, MAIN_SOCK_EVENT);
	return true;
}

bool deal_single_new_connection(int fd)
{
	int new_fd;
	struct sockaddr_in addr;
	socklen_t length = sizeof(addr);

	new_fd = accept(fd, (struct sockaddr *)&addr, &length);

	if (new_fd < 0)
	{
		return false;
	}
	add_read_event(main_kq, new_fd, 0, UNSURE_SOCK_TYPE);
	return true;
}

void deal_new_connection(int fd)
{
	int i = 0, max_accept = 50;
	for (i = 0; i < max_accept; ++i)
	{
		if (!deal_single_new_connection(fd))
		{
			break;
		}
	}
}

void set_fd_event_type(int fd)
{
	unsigned short type;
	size_t size;
	char gdebug[10240];
	size = read(fd, &type, sizeof(type));
	if (size != sizeof(type))
	{
		close(fd);
		return;
	}
	serial_string(gdebug, "read type %d\n", type);
	log_string("debug.log", gdebug);
	if (type == GS_SERVER_CONNECT)
	{
		remove_read_event(main_kq, fd);
		add_read_event(main_kq, fd, 0, GS_EVENT);
	}
	else
	{
	}
}

int main_loop()
{
	int nevents;
	int i = 0;
	while (true)
	{
		nevents = kevent(main_kq, NULL, 0, main_event, sizeof(main_event) / sizeof(main_event[0]), NULL);
		for (i = 0; i < nevents; ++i)
		{
			long udata = (long)main_event[i].udata;
			int fd = main_event[i].ident;
			if (main_event[i].filter == EVFILT_READ)
			{
				switch (udata)
				{
				case MAIN_SOCK_EVENT:
					deal_new_connection(fd);
					break;
				case UNSURE_SOCK_TYPE:
					set_fd_event_type(fd);
					break;
				case GS_EVENT:
					deal_gs_event(fd);
					break;
				}
			}
		}

		deal_gs_buff();
	}
}

int main()
{
	if (!init_main_event())
	{
		return -1;
	}
	if (!init_main_socket())
	{
		return -1;
	}
	if (!init_all_module())
	{
		return -1;
	}
	main_loop();
}
