/*  shmcat.c      cat content of shared memory and unlink memory
    shmcat memname blocksize
    gcc -Wall -O0 -o shmcat shmcat.c -lrt
*/

#include "version.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "cprefresh.h"

void usage( ) {
  fprintf(stderr,
          "shmcat (version %s of frankl's stereo utilities",
          VERSION);
  fprintf(stderr, ")\nUSAGE:\n");
  fprintf(stderr,
"\n"
" shmcat --shmname=memname [--block-size=blksize]\n"
" shmcat -i memname [-b blksize]\n"
"\n"
"  This program writes the content of a shared memory file memname in blocks\n"
"  of  blksize bytes to stdout. The shared memory file is deleted afterwards.\n"
"\n"
"  The default for blksize is 8192.\n"
"\n"
"  You may create the shared memory file with 'cptoshm'.\n"
"\n"
"  The program also support the '--version' option to display its version\n"
"  and the '--verbose' option to display when it is starting.\n"
"\n"
);
}

int main(int argc, char *argv[])
{
  char *memname;
  int fd, optc, verbose;
  size_t length, blen, done, wlen;
  struct stat sb;
  char *mem, *ptr;

  /* read command line options */
  static struct option longoptions[] = {
      {"shmname", required_argument, 0, 'i' },
      {"block-size", required_argument, 0,  'b' },
      {"verbose", no_argument, 0, 'v' },
      {"version", no_argument, 0, 'V' },
      {"help", no_argument, 0, 'h' },
      {0,         0,                 0,  0 }
  };

  if (argc == 1) {
     usage();
     exit(0);
  }
  blen = 8192;
  memname = NULL;
  verbose = 0;
  while ((optc = getopt_long(argc, argv, "i:b:Vh",
          longoptions, &optind)) != -1) {
      switch (optc) {
      case 'i':
        memname = optarg;
        break;
      case 'b':
        blen = atoi(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      case 'V':
        fprintf(stderr,
                "shmcat (version %s of frankl's stereo utilities)\n",
                VERSION);
        exit(0);
      default:
        usage();
        exit(0);
      }
  }

  if (memname == NULL) {
      fprintf(stderr, "shmcat: need --shmname argument . . . bye.\n");
      exit(7);
  }
  if ((fd = shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
      fprintf(stderr, "shmcat: Cannot open memory %s.\n", memname);
      exit(3);
  }
  if (fstat(fd, &sb) == -1) {
      fprintf(stderr, "shmcat: Cannot stat shared memory %s.\n", memname);
      exit(4);
  }
  length = sb.st_size;
  mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
      fprintf(stderr, "shmcat: Cannot map shared memory.\n");
      exit(6);
  }
  ptr = mem;
  done = 0;
  if (verbose)
      fprintf(stderr, "shmcat: starting");
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
      fprintf(stderr, "shmcat: Cannot unlink shared memory.\n");
      exit(7);
  }
  exit(0);
}

