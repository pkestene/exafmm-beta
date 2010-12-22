#include <sys/time.h>

double get_time(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec+tv.tv_usec*1e-6));
}
