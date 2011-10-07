#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdbool.h>
#include "rbuff.h"

typedef int (*protocol_dealer)(unsigned short len, char *data);
static lua_State *log_state;
extern char gdebug[10240];

static rbuff_t gs_buff;

bool init_gs_module()
{
	if (rbuff_allocate(&gs_buff, 4096) < 0)
	{
		log_and_quit(3, "can't initial gs_buff");
	}
	log_state = luaL_newstate();

	luaL_openlibs(log_state);
	luaL_dofile(log_state, "log_module.lua");
	return true;
}

int game_server_log(unsigned short len, char *data)
{
	lua_getglobal(log_state, "parse_log");
	lua_pushlstring(log_state, data, len);
	lua_call(log_state, 1, 0);
}

protocol_dealer dealers[] = {
	NULL,
	game_server_log,
};

void deal_gs_event(int fd)
{
	char buff[10240];
	int size;
	int type;
	size = read(fd, buff, sizeof(buff));
	if (size <= 0)
	{
		remove_read_event_from_main_kqueue(fd);
		close(fd);
		return;
	}
	safe_rbuff_dpush_raw(&gs_buff, buff, size);
}

void deal_gs_buff()
{
	unsigned short length = 0;
	unsigned short type = 0;
	char data[10240];
	int i = 0, max_deal = 256;
	if (rbuff_size(&gs_buff) == 0)
	{
		return;
	}
	for (i = 0; i < max_deal; ++i)
	{
		type = safe_pop_gs_packet(&gs_buff, data, &length);
		if (length == 0 || type == 0)
		{
			return;
		}
		if (sizeof(dealers) / sizeof(dealers[0]) <= type)
		{
			continue;
		}
		if (dealers[type] == NULL)
		{
			continue;
		}
		(dealers[type])(length - sizeof(type), data);
	}
}
