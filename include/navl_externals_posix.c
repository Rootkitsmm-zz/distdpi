#include <navl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

int null_log_message(const char *level, const char *func, const char *format, ... )
{
	return 0;
}

int printf_log_message(const char *level, const char *func, const char *format, ... )
{
	int res = 0;
	char buf[4096];
	va_list va;
	va_start(va, format);

	res = snprintf(buf, 4096, "%s: %s: ", level, func);
	res += vsnprintf(buf + res, 4096 - res, format, va);
	navl_diag_printf(buf);
	va_end(va);
	return res;
}

void bind_navl_externals()
{
	/* memory allocation */
	navl_malloc_local = malloc;
	navl_free_local = free;
	navl_malloc_shared = malloc;
	navl_free_shared = free;
	
	/* string functions */
	navl_memcpy = memcpy;
	navl_memcmp = memcmp;
	navl_memset = memset;
	navl_strcasecmp = strcasecmp;
	navl_strchr = (const char* (*)(const char*, int))strchr;
	navl_strcmp = strcmp;
	navl_strncmp = strncmp;
	navl_strcpy = strcpy;
	navl_strncpy = strncpy;
	navl_strlen = strlen;
	navl_strstr = (const char* (*)(const char*, const char*))strstr;

	/* input/output */
	navl_printf = printf;
	navl_sprintf = sprintf;
	navl_snprintf = snprintf;
	navl_sscanf = sscanf;
	navl_putchar = putchar;
	navl_puts = puts;
	navl_diag_printf = printf;

	/* time */
	navl_gettimeofday = (int (*)(struct navl_timeval*, void*))gettimeofday;
	navl_mktime = (navl_time_t (*)(struct navl_tm*))mktime;

	/* system */
	navl_abort = abort;
	navl_get_thread_id = (unsigned long (*)(void))pthread_self;

	/* navl specific */
#ifdef DEBUG
	navl_log_message = printf_log_message;
#else
	navl_log_message = null_log_message;
#endif

// The following externals are being deprecated. For compatibility they are 
// currently still being set.
#define NAVL_4_2_COMPATIBILITY 1

#ifdef NAVL_4_2_COMPATIBILITY
	/* ctype */
	navl_islower = islower;
	navl_isupper = isupper;
	navl_tolower = tolower;
	navl_toupper = toupper;
	navl_isalnum = isalnum;
	navl_isspace = isspace;
	navl_isdigit = isdigit;

	/* string functions */
	navl_atoi = atoi;
	navl_strrchr = (const char* (*)(const char*, int))strrchr;
	navl_strerror = strerror;
	navl_strftime = (size_t (*)(char*, size_t, const char*, const struct navl_tm*))strftime;
	navl_strpbrk = (const char* (*)(const char*, const char*))strpbrk;
	navl_strtol = strtol;

	/* math */
	navl_log = log;
	navl_fabs = fabs;
#endif
}

