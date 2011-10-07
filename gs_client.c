#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "protocol.h"

#define HEART_BEAT_IDENT -1
#define HEART_BEAT_INTERVAL 1000
static struct kevent gs_event_list[256];
static int gs_kq;
static lua_State *gs_test_state;
static FILE *out_put;
char gdebug[10240];
static int gs_fd;
static bool send_type = false;
void send_gs_data(unsigned short type, const char *data, unsigned short len);

static int c_send_log(lua_State *L)
{
	const char *data = luaL_checkstring(L, 1);
	send_gs_data(SERVER_LOG, data, strlen(data));
	return 0;
}

static const struct luaL_Reg gs_net []  = {
	{"c_send_log", c_send_log},
	{NULL, NULL},
};

void init_gs_env()
{
	struct kevent ev;
	int result;

	gs_kq = kqueue();
	if (gs_kq == -1)
	{
		serial_string(gdebug, "kqueue %d\n", gs_kq);
		log_string("client.log", gdebug);
	}

	EV_SET(&ev, HEART_BEAT_IDENT, EVFILT_TIMER, EV_ADD, 0, HEART_BEAT_INTERVAL, NULL);
	result = kevent(gs_kq, &ev, 1, NULL, 0, NULL);
	if (result < 0)
	{
		serial_string(gdebug, "add event %d\n", result);
		log_string("client.log", gdebug);
	}

	gs_test_state = luaL_newstate();
	luaL_openlibs(gs_test_state);
	luaL_register(gs_test_state, "gs_net", gs_net);
	luaL_dofile(gs_test_state, "gs_test_module.lua");
}

void send_gs_data(unsigned short type, const char *data, unsigned short len)
{
	gs_data_t send_data;
	if (!send_type)
	{
		send_type = true;
		type = GS_SERVER_CONNECT;
		send(gs_fd, &type, sizeof(type), 0);
	}
	send_data.type = type;
	send_data.len = len;
	strlcpy(send_data.data, data, 10240);
	send(gs_fd, &send_data, sizeof(type) + sizeof(len) + len, 0);
}

void heart_beat()
{
	lua_getglobal(gs_test_state, "heart_beat");
	lua_call(gs_test_state, 0, 0);
}

void gs_env_loop()
{
	int nevents;
	int i = 0;
	bool heart_beat_enable = false;

	while (true)
	{
		nevents = kevent(gs_kq, NULL, 0, gs_event_list, sizeof(gs_event_list) / sizeof(gs_event_list[0]), NULL);
		for (i = 0; i < nevents; ++i)
		{
			if (gs_event_list[i].filter = EVFILT_TIMER)
			{
				if (gs_event_list[i].ident == HEART_BEAT_IDENT)
				{
					heart_beat_enable = true;
				}
			}
		}
		if (heart_beat_enable)
		{
			heart_beat_enable = false;
			heart_beat();
		}
	}
}

int main()
{
	int port = get_main_port();
	unsigned short type;
	gs_fd = connect_service("127.0.0.1", port);
	if (gs_fd < 0)
	{
		serial_string(gdebug, "connect fail %d\n", gs_fd);
		log_string("client.log", gdebug);
		return -1;
	}
	init_gs_env();
	gs_env_loop();
}
