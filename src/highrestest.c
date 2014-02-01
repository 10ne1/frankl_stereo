/*
highrestest.c                Copyright frankl 2013-2014

This file is part of frankl's stereo utilities. 
See the file License.txt of the distribution and 
http://www.gnu.org/licenses/gpl.txt for license details.
*/
#include <time.h>
#include <stdio.h>

/* a simple test of the resolution of several CLOCKs */
int main() {
  int ret, highresok;
  struct timespec res, tim;

  ret = clock_getres(CLOCK_REALTIME, &res);
  printf("realtime res: %ld s %ld ns (%d)\n", res.tv_sec, res.tv_nsec, ret); 

  ret = clock_gettime(CLOCK_REALTIME, &tim);
  printf("realtime: %ld s %ld ns (%d)\n", tim.tv_sec, tim.tv_nsec, ret); 

  ret = clock_getres(CLOCK_MONOTONIC, &res);
  highresok = (res.tv_sec == 0 && res.tv_nsec == 1);
  printf("monotonic res: %ld s %ld ns (%d)\n", res.tv_sec, res.tv_nsec, ret); 

  ret = clock_gettime(CLOCK_MONOTONIC, &tim);
  printf("monotonic: %ld s %ld ns (%d)\n", tim.tv_sec, tim.tv_nsec, ret); 

  ret = clock_getres(CLOCK_PROCESS_CPUTIME_ID, &res);
  printf("process res: %ld s %ld ns (%d)\n", res.tv_sec, res.tv_nsec, ret); 

  ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tim);
  printf("process: %ld s %ld ns (%d)\n", tim.tv_sec, tim.tv_nsec, ret); 

  if (highresok) 
     printf("\nHighres timer seems enabled!\n");
  else
     printf("\nHighres is NOT enabled!\n");

  return 0;

}
