#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>

char gdebug[10240];
void serial_string(char *buf, const char *fmt, ...)
{
	char strtime[64];
	char tmp_buf[10240 - 64];
	va_list ap;
	time_t timer = time(NULL);
	struct tm* timeptr;

	timeptr = localtime(&timer);
	strftime(strtime, 30, "[%Y-%m-%d %H:%M:%S] ", timeptr);

	va_start(ap, fmt);
	vsnprintf(tmp_buf, sizeof(tmp_buf), fmt, ap);
	va_end(ap);

	snprintf(buf, 10240, "%s %s", strtime, tmp_buf);
}

void log_string(char *filename, char buf[10240])
{
	FILE *logfile = fopen(filename, "a");
	fwrite(buf, strlen(buf), 1, logfile);
	fclose(logfile);
}

void hex_string(char buf[10240], char *data, int len)
{
	int pos = 0;
	int i = 0;
	bzero(buf, 10240);
	for (i = 0; i < len; ++i)
	{
		pos += snprintf(buf + pos, 10240 - pos, "%X ", data[i]);
	}
	pos += snprintf(buf + pos, 10240 - pos, "\n");
}
