#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <assert.h>
#include "log.h" 


#define ESC  "\033"
#define RST ESC"[m"
#define FONT(ef, fg) ESC"["#ef";"#fg"m"
#define SFONT(str, ef, fg) FONT(ef, fg) str RST //transition macro, make use of the rule of the macro-expansion

#define BOLD  1
#define GREEN  32
#define Log(fmt, ...) \
	_Log(SFONT("[%s,%d,%s]", BOLD, GREEN) fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)


#define Assert(cond, ...) \
 do { \
   if (!(cond)) { \
     fflush(stdout); \
	 fprintf(stderr, __VA_ARGS__); \
	 assert(cond); \
   } \
 } while (0)

#endif
