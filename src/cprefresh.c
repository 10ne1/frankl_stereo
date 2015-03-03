/*
cprefresh.c                Copyright frankl 2013-2015

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.

Here are some utility functions to clean or refresh RAM.
Optionally, they use assembler functions on ARM.
*/


#ifdef VFPREFRESH
/* on ARM with some version of the CPU feature 'vfp' */
inline void refreshmem(char* ptr, int n)
{
  /* we make shure below that we can access ptr-off */
  int off;
  off = (unsigned int)ptr % 8;
  memcp_vfpX((void*)(ptr-off), (void*)(ptr-off), ((n+off)/8)*8);
}
inline void memclean(char* ptr, int n)
{
  int i, off, n0;
  off = (unsigned int)ptr % 8;
  if (off > 0) {
    off = 8-off;
    for (i=0; i < off; i++) ptr[i] = 0;
  }
  n0 = ((n-off)/8)*8;
  memclean_vfpX((void*)(ptr+off), n0);
  for (; n0+off < n; n0++) ptr[n0+off] = 0;
}

#else
#ifdef ARMREFRESH
/* on ARM with no CPU feature 'neon' or 'vfp' */

inline void refreshmem(char* ptr, int n)
{
  /* we make shure below that we can access ptr-off */
  int off;
  off = (unsigned int)ptr % 4;
  memcp_regX((void*)(ptr-off), (void*)(ptr-off), ((n+off)/4)*4);
}
inline void memclean(char* ptr, int n)
{
  int i, off, n0;
  off = (unsigned int)ptr % 4;
  if (off > 0) {
    off = 4-off;
    for (i=0; i < off; i++) ptr[i] = 0;
  }
  n0 = ((n-off)/4)*4;
  memclean_regX((void*)(ptr+off), n0);
  for (; n0+off < n; n0++) ptr[n0+off] = 0;
}

#else

#include <stdint.h>
/* default version in C, compile with -O0, such that this is not
   optimized away */
inline void refreshmem(char* ptr, int n)
{
  /* we make shure below that we can access ptr-off */
  int i, sz, off;
  unsigned int x, d, *up;
  sz = sizeof(unsigned int);
  off = (uintptr_t)ptr % sz;
  for(i=0, up=(unsigned int*)(ptr-off); i < (n+off)/sz; i++) {
      x = *up;
      d = 0xFFFFFFFF;
      x = x ^ d;
      x = x ^ d;
      *up = x;
  }
}
inline void memclean(char* ptr, int n)
{
  int i, off, n0, sz;
  unsigned int *up;
  sz = sizeof(int);
  off = (uintptr_t)ptr % sz;
  if (off > 0) {
    off = sz-off;
    for (i=0; i < off; i++) ptr[i] = 0;
  }
  n0 = (n-off)/sz;
  for(up = (unsigned int*)(ptr+off), i=0; i < n0; i++) {
      *up = 0xFFFFFFFF;
      *up = 0;
      *up = 0;
  }
  n0 *= sz;
  for (; n0+off < n; n0++) ptr[n0+off] = 0;
}


#endif
#endif





