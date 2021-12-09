#include <stdio.h>
#include <logsystem/debug.h>
#include <stdarg.h>
FILE *log_fp = NULL;

void init_log(const char *log_file) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w"); //"w - truncate file to zero length or create text file for writing."
  Assert(log_fp, "Can not open '%s'", log_file);
}

#define MAXN 10000
int log_enable() {
  static int call_nr = 0;
  if (call_nr < MAXN) {
    call_nr++;  
	return 1;
  }
  return 0;
}
