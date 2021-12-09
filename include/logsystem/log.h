#ifndef __EASY_LOG_H__
#define __EASY_LOG_H__

#include <stdio.h>

extern FILE* log_fp;
#define log_write(...) \
  do { \
    extern int log_enable(); \
	if (log_fp != NULL && log_enable()) { \
	  fprintf(log_fp, __VA_ARGS__); \
/*after invoke `fprintf`, the position of file stream will move forward.*/ \
	  fflush(log_fp); \
/*for output stream, it forces a write of all user-space buffered data for the given output or update steram */ \
	} \
  } while(0)

#define _Log(...) \
  do { \
	printf(__VA_ARGS__); \
    log_write(__VA_ARGS__); \
  } while(0)

void init_log(const char *);
#endif
