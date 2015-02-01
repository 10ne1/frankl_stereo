/*
cprefresh.h                Copyright frankl 2013-2015

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.

Here are some utility functions to clean or refresh RAM.
Optionally, they use assembler functions on ARM.
*/


#ifdef ARMREFRESH

inline void memcp_regX(void*, void*, int);
inline void memclean_regX(void*, int);

#endif

#ifdef VFPREFRESH

inline void memcp_vfpX(void*, void*, int);
inline void memclean_vfpX(void*, int);

#endif

inline void refreshmem(char* ptr, int n);
inline void memclean(char* ptr, int n);

