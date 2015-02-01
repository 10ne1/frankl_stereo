/*  shmcat.c      cat content of shared memory and unlink memory
    shmcat memname blocksize
    gcc -Wall -O0 -o shmcat shmcat.c -lrt
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "version.h"
#include "cprefresh.h"

void usage( ) {
  fprintf(stderr,
          "shmcat (version %s of frankl's stereo utilities",
          VERSION);
  fprintf(stderr, ")\nUSAGE:\n");
  fprintf(stderr,
"\n"
" shmcat memname blksize\n"
"\n"
"  This program writes the content of a shared memory file memname in blocks\n"
"  of size blksize to stdout. The shared memory file is deleted afterwards.\n"
"\n"
"  You may create the shared memory file with 'cptoshm'.\n"
"\n"
);
}

int main(int argc, char *argv[])
{
  char *memname;
  int fd;
  size_t length, blen, done, wlen;
  struct stat sb;
  char *mem, *ptr;

  if (argc != 3) {
     usage();
     exit(0);
  }

  memname = argv[1];
  blen = atoi(argv[2]);
  if ((fd = shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
      fprintf(stderr, "Cannot open memory %s.\n", memname);
      exit(3);
  }
  if (fstat(fd, &sb) == -1) {
      fprintf(stderr, "Cannot stat shared memory %s.\n", memname);
      exit(4);
  }
  length = sb.st_size;
  mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
      fprintf(stderr, "Cannot map shared memory.");
      exit(6);
  }
  ptr = mem;
  done = 0;
  while (done < length-blen) {
      refreshmem(ptr, blen);
      refreshmem(ptr, blen);
      wlen = write(1, ptr, blen);
      done += wlen;
      ptr += wlen;
  }
  while (done < length) {
      wlen = write(1, ptr, length-done);
      done += wlen;
      ptr += wlen;
  }
  if (shm_unlink(memname) == -1) {
      fprintf(stderr, "Cannot unlink shared memory.");
      exit(7);
  }
  exit(0);
}

