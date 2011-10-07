//$Id: rbuff.h 188074 2011-02-09 02:27:33Z richardcao@NETEASE.COM $
#ifndef _RBUFF_H_
#define _RBUFF_H_

#include <pthread.h>
#define STATIC_BUF (1)
#define DYNAMIC_BUF (2)
typedef struct {
	unsigned int len;
	unsigned int start;
	unsigned int end;
	unsigned int buf_type;
	pthread_mutex_t mutex;
	char *data;
} rbuff_t;

int rbuff_assign(rbuff_t *buf, char *data, unsigned int len);
int  rbuff_allocate(rbuff_t *buf, unsigned int len);
void rbuff_free(rbuff_t *buf);
int  rbuff_len(rbuff_t *buff);

int   rbuff_size(rbuff_t *buf);
char *rbuff_data(rbuff_t *buf);

//int rbuff_push_raw(rbuff_t *buf, const char *s, unsigned int len);
//int rbuff_dpush_raw(rbuff_t *buf, const char *s, unsigned int len);
//int rbuff_pop_raw(rbuff_t *buf, char *data, unsigned int len);

int safe_rbuff_push_raw(rbuff_t *buf, const char *s, unsigned int len);
int safe_rbuff_dpush_raw(rbuff_t *buf, const char *s, unsigned int len);
int safe_rbuff_pop_raw(rbuff_t *buf, char *data, unsigned int len);

int  safe_pop_packet(rbuff_t *buff, char *dst);
void safe_push_add_message(rbuff_t *mb, const char *data);

void rbuff_lock(rbuff_t *buf);
void rbuff_unlock(rbuff_t *buf);

#endif //_RBUFF_H_
