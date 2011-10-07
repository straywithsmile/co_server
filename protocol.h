#ifndef __RPOTOCOL_H__
#define __PROTOCOL_H__

//-----event type---
#define MAIN_SOCK_EVENT 1
#define GS_EVENT 2
#define UNSURE_SOCK_TYPE 3

//-----connecting server type--
#define GS_SERVER_CONNECT 3

//-----gs to co protocol---
#define SERVER_LOG 1
#define DB_QUERY 2

typedef struct {
	short type;
	short len;
	char data[10240];
}gs_data_t;
#endif
