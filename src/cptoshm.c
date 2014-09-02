/*  cptoshm.c     copy file into shared memory 
    cptoshm filename memname         (memname should start with /)
    gcc -Wall -O0 -o cptoshm cptoshm.c -lrt
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define BUFSIZE 4000000
int main(int argc, char *argv[])
{
  char *infile, *memname;
  int ifd, fd;
  size_t length, done, rlen;
  struct stat sb;
  char buf[BUFSIZE], *mem, *ptr;
  infile = argv[1];
  if ((ifd = open(infile, O_RDONLY)) == -1) {
      fprintf(stderr, "Cannot open input file %s.\n", infile);
      exit(2);
  }
  if (fstat(ifd, &sb) == -1) {
      fprintf(stderr, "Cannot stat input file %s.\n", infile);
      exit(4);
  }
  length = sb.st_size;
  memname = argv[2];
  if ((fd = shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
      fprintf(stderr, "Cannot open memory %s.\n", memname);
      exit(3);
  }
  if (ftruncate(fd, length) == -1) {
      fprintf(stderr, "Cannot truncate to %d.", length);
      exit(5);
  }
  mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
      fprintf(stderr, "Cannot map shared memory.");
      exit(6);
  }
  ptr = mem;
  done = 0;
  while (done < length) {
      rlen = read(ifd, buf, BUFSIZE);
      memcpy(ptr, buf, rlen);
      done += rlen;
      ptr += rlen;
  }
  exit(0);

}
