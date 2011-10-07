#include "rbuff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_ALIGN (255)
#define ROUNDUP_SIZE(_r) (((_r) + MEM_ALIGN) & ~MEM_ALIGN)
//#define TEST_MARSH
//#define DEBUG_MEM

#ifdef DEBUG_MEM
static unsigned long check_value = (unsigned long)-1;
#define TAIL_LEN (sizeof(unsigned long))
#define BUFF_TAIL_SET(buf) memcpy((buf)->data + (buf)->len, (char *)(&check_value), TAIL_LEN)
#define BUFF_CHECK(buf) do { \
	if (memcmp((buf)->data + (buf)->len, (&check_value), TAIL_LEN) != 0) \
	{ \
		printf("buffer is bad %d %ld\n", __LINE__, *(unsigned long *)((buf)->data + (buf)->len)); \
	} \
	if ((buf)->end < 0 || (buf)->len < (buf)->end) \
	{ \
		printf("buffer tail wrong %d len %d end %d\n", __LINE__, (buf)->len, (buf)->end); \
	} \
	if ((buf)->start < 0 || (buf)->len < (buf)->start) \
	{ \
		printf("buff head wrong %d\n", __LINE__); \
	} \
	if ((buf)->end < (buf)->start) \
	{ \
		printf("head and end switch wrong %d\n", __LINE__); \
	} \
} while(0)
#else
#define TAIL_LEN (0)
#define BUFF_TAIL_SET(buf)
#define BUFF_CHECK(buf)
#endif

int rbuff_allocate(rbuff_t *buf, unsigned int len)
{
	buf->data = (char *)malloc(len + TAIL_LEN);
	if (buf->data == NULL)
	{
		return -1;
	}
	bzero(buf->data, len + TAIL_LEN);
	buf->start = 0;
	buf->end = 0;
	pthread_mutex_init(&buf->mutex, NULL);
	buf->len = len;
	buf->buf_type = DYNAMIC_BUF;
	BUFF_TAIL_SET(buf);
	return 0;
}

int rbuff_assign(rbuff_t *buf, char *data, unsigned int len)
{
	if (buf->data != NULL)
	{
		return -1;
	}
	buf->data = data;
	buf->len = len - TAIL_LEN;
	buf->start = 0;
	buf->end = 0;
	pthread_mutex_init(&buf->mutex, NULL);
	buf->buf_type = STATIC_BUF;
	BUFF_TAIL_SET(buf);
	return 0;
}

void rbuff_free(rbuff_t *buf)
{
	if (buf->data != NULL)
	{
		free(buf->data);
	}
	buf->data = NULL;
	buf->start = 0;
	buf->end = 0;
	buf->len = 0;
	pthread_mutex_destroy(&buf->mutex);
}

static inline int rbuff_align(rbuff_t *buf)
{
	if (buf->start == 0)
	{
		return 0;
	}
	memmove(buf->data, buf->data + buf->start, buf->end - buf->start);
	buf->end -= buf->start;
	buf->start = 0;
	BUFF_CHECK(buf);
	return 1;
}

int rbuff_size(rbuff_t *buf)
{
	return buf->end - buf->start;
}

int rbuff_left_size(rbuff_t *buf)
{
	return buf->start + (buf->len - buf->end);
}

char *rbuff_data(rbuff_t *buf)
{
	return buf->data + buf->start;
}

int rbuff_len(rbuff_t *buf)
{
	return buf->len;
}

static int rbuff_grow(rbuff_t *buf, int size)
{
	char *tmp;
	int need_size;
	if (buf->buf_type == STATIC_BUF)
	{
		return -1;
	}
	rbuff_align(buf);
	need_size = ROUNDUP_SIZE(rbuff_size(buf) + size + TAIL_LEN);
	tmp = (char *)realloc(buf->data, need_size);
	if (tmp == 0)
	{
		return -1;
	}
	buf->len = need_size;
	buf->data = tmp;
	BUFF_TAIL_SET(buf);
	BUFF_CHECK(buf);
	return 0;
}

static inline int check_buff(rbuff_t *buf, int push_size, int pop_size)
{
	if (buf == NULL)
	{
		return 0;
	}
	if (buf->data == NULL)
	{
		return 0;
	}
	if (push_size > 0)
	{
		if (rbuff_left_size(buf) < push_size)
		{
			return 0;
		}
	}
	if (pop_size > 0)
	{
		if (rbuff_size(buf) < pop_size)
		{
			return 0;
		}
	}
	return 1;
}

static inline void _push_raw(rbuff_t *buf, const char *s, unsigned int len)
{
	rbuff_align(buf);
	memcpy(buf->data + buf->end, s, len);
	buf->end += len;
	BUFF_CHECK(buf);
}

void rbuff_lock(rbuff_t *buf)
{
	pthread_mutex_lock(&buf->mutex);
}

void rbuff_unlock(rbuff_t *buf)
{
	pthread_mutex_unlock(&buf->mutex);
}

#define DECLARE_RBUFF_PUSH_INT(bit, type)\
	int rbuff_push_int##bit(rbuff_t *buf, type s)\
{\
	if (!check_buff(buf, sizeof(type), 0)) {\
		return -1;\
	}\
	_push_raw(buf, (const char *)&s, sizeof(type));\
	return 0;\
}

#define DECLARE_RBUFF_DPUSH_INT(bit, type)\
	int rbuff_dpush_int##bit(rbuff_t *buf, type s)\
{\
	if (!check_buff(buf, 0, 0)) {\
		return -1;\
	}\
	if (rbuff_left_size(buf) < sizeof(s)) \
	{ \
		if (rbuff_grow(buf, sizeof(s)) < 0) \
		{ \
			return -1; \
		} \
	} \
	_push_raw(buf, (const char *)&s, sizeof(type)); \
	return 0;\
}

#define DECLARE_RBUFF_POP_INT(bit, type)\
	int rbuff_pop_int##bit(rbuff_t *buf, type *d)\
{\
	if (!check_buff(buf, 0, sizeof(type))) {\
		return -1;\
	}\
	_pop_raw(buf, (char *)d, sizeof(type));\
	return 0;\
}

int rbuff_push_raw(rbuff_t *buf, const char *s, unsigned int len)
{
	if (!check_buff(buf, len, 0))
	{
		return -1;
	}
	rbuff_align(buf);
	_push_raw(buf, s, len);
	return 0;
}

int safe_rbuff_push_raw(rbuff_t *buf, const char *s, unsigned int len)
{
	int result;
	rbuff_lock(buf);
	result = rbuff_push_raw(buf, s, len);
	rbuff_unlock(buf);
	return result;
}

int rbuff_dpush_raw(rbuff_t *buf, const char *s, unsigned int len)
{
	if (!check_buff(buf, 0, 0))
	{
		return -1;
	}
	rbuff_align(buf);
	if (rbuff_left_size(buf) < len)
	{
		if (rbuff_grow(buf, len) < 0)
		{
			return -1;
		}
	}
	_push_raw(buf, s, len);
	BUFF_CHECK(buf);
	return 0;
}

int safe_rbuff_dpush_raw(rbuff_t *buf, const char *s, unsigned int len)
{
	int result;
	rbuff_lock(buf);
	result = rbuff_dpush_raw(buf, s, len);
	rbuff_unlock(buf);
	return result;
}

static inline void _pop_raw(rbuff_t *buf, char *dst, unsigned int len)
{
	if (dst != NULL)
	{
		memcpy(dst, buf->data + buf->start, len);
	}
	buf->start += len;
	if (buf->start == buf->end)
	{
		buf->start = 0;
		buf->end = 0;
	}
	BUFF_CHECK(buf);
}

int rbuff_pop_raw(rbuff_t *buf, char *dst, unsigned int len)
{
	if (!check_buff(buf, 0, len))
	{
		return -1;
	}
	_pop_raw(buf, dst, len);
	return 0;
}

int safe_rbuff_pop_raw(rbuff_t *buf, char *dst, unsigned int len)
{
	int result;
	rbuff_lock(buf);
	result = rbuff_pop_raw(buf, dst, len);
	rbuff_unlock(buf);
	return result;
}

//
int pop_packet(rbuff_t *buf, char *dst, unsigned short *pop_len)
{
	unsigned short type = 0;
	unsigned short len = 0;
	char debugmsg[10240];
	if (rbuff_size(buf) < sizeof(type) + sizeof(len))
	{
		return 0;
	}
	type = *((unsigned short *)rbuff_data(buf));
	len  = *((unsigned short *)(rbuff_data(buf) + sizeof(type)));

	if (!check_buff(buf, 0, len + sizeof(type) + sizeof(len)))
	{
		hex_string(debugmsg, rbuff_data(buf), rbuff_size(buf));
		log_string("debug.log", debugmsg);
		return 0;
	}
	_pop_raw(buf, NULL, sizeof(type) + sizeof(len));
	_pop_raw(buf, dst, len);
	*pop_len = len;
	return type;
}

unsigned short safe_pop_gs_packet(rbuff_t *buf, char *dst, unsigned short *len)
{
	unsigned short type;
	rbuff_lock(buf);
	type = pop_packet(buf, dst, len);
	rbuff_unlock(buf);
	return type;
}

#ifdef TEST_MARSH
#include <stdio.h>
int main(int arg, char * argv[])
{
	rbuff_t mb;
	char buf[256];
	char *src = "LKJlfsjdflsdfjsfjsdfjajfasf";
	char *src2 = "LKKlfsjdflsdfjsfjsdfjajfasf";
	int i =0;
	rbuff_allocate(&mb, 1);
	/*
	for(i = 0; i < 10; ++i)
	{
		s_push_raw(&mb, src, strlen(src));
		s_push_raw(&mb, " ", 1);
		s_push_raw(&mb, src, strlen(src));
		s_push_raw(&mb, "\n", 1);
	}
	printf("%s %d %d\n", m_data(&mb), m_size(&mb), mb.len);
	for (i = 0; i < 5; ++i)
	{
		pop_raw(&mb, buf, 28);
		pop_raw(&mb, buf, 28);
	}
	printf("%s %d %d\n", m_data(&mb), m_size(&mb), mb.len);
	*/
	/*
	for (i = 0; i < 600; ++i)
	{
		s_push_raw(&mb, src2, strlen(src2));
		s_push_raw(&mb, " ", 1);
		s_push_raw(&mb, src2, strlen(src2));
		s_push_raw(&mb, "\n", 1);
	}
	for (i = 0; i < 500; ++i)
	{
		pop_raw(&mb, buf, 28);
		pop_raw(&mb, buf, 28);
	}
	for (i = 0; i < 600; ++i)
	{
		s_push_raw(&mb, src2, strlen(src2));
		s_push_raw(&mb, " ", 1);
		s_push_raw(&mb, src2, strlen(src2));
		s_push_raw(&mb, "\n", 1);
	}
	push_int8(&mb, 0);
	for (i = 0; i < 698; ++i)
	{
		pop_raw(&mb, buf, 28);
		pop_raw(&mb, buf, 28);
	}
	*/
	//printf("%s %d %d buf start %d end %d len %d\n", m_data(&mb), m_size(&mb), mb.len, mb.start, mb.end, strlen(m_data(&mb)));
	printf("size %d\n", rbuff_size(&mb));
	push_add_message(&mb, "1234567890");
	printf("size %d\n", rbuff_size(&mb));
	push_add_message(&mb, "123\n456\n789");
	printf("size %d\n", rbuff_size(&mb));
	printf("%s %d %d buf start %d end %d len %d\n", rbuff_data(&mb), rbuff_size(&mb), mb.len, mb.start, mb.end, strlen(rbuff_data(&mb)));
	printf("size %d\n", rbuff_size(&mb));
	for (i = 0; i < 698; ++i)
	{
		rbuff_dpush_raw(&mb, "hello world!", strlen("hello world!"));
	}
	printf("size %d len %d\n", rbuff_size(&mb), rbuff_len(&mb));
	//pop_int32(&mb, &tmp);
	//printf("%s %d %d\n", m_data(&mb), m_size(&mb), mb.len);
	rbuff_free(&mb);
}
#endif
