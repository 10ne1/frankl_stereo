/*  cptoshm.c     copy file into shared memory 
    gcc -Wall -O0 -o cptoshm cptoshm.c -lrt
*/
#include "version.h"
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
"cptoshm --shmname=/<name> [options]\n"
"\n"
"This program copies the content of a file (or of stdin) into a shared \n"
"memory area. \n"
"\n"
"USAGE HINTS\n"
"\n"
"The converse is done by 'shmcat'. This can be used for audio playback \n"
"by copying music files from hard disc to memory before playing them.\n"
"This approach causes less operating system overhead compared to copying \n"
"music files to a ramdisk.\n"
"\n"
"OPTIONS\n"
"\n"
"--file=name, -i name\n"
"    the name of the input file. If this is not given input is taken\n"
"    from stdin (and in this case you need to specify --max-input).\n"
"\n"
"--shmname=string, -o string\n"
"    the name of the shared memory area to which the input is written.\n"
"    By convention this name should start with a slash (/).\n"
"\n"
"--buffer-size=intval, -b intval\n"
"    the internal buffer size in bytes. The default is about 4 MB.\n"
"\n"
"--max-input=intval, -m intval\n"
"    this is the maximal number of bytes read from the input file.\n"
"    If input is a file the default is the length of that file.\n"
"    If input is stdin then this  option must be given.\n"
"\n"
"--overwrite=intval, -O intval\n"
"    the number of times the buffer is cleared by overwriting with\n"
"    certain bit patterns and zeroes. Default is 0.\n"
"\n"
"--verbose, -v\n"
"    print some information during startup and operation.\n"
"\n"
"--version, -V\n"
"    print information about the version of the program and abort.\n"
"\n"
"--help, -h\n"
"    print this help page and abort.\n"
"\n"
"EXAMPLES\n"
"\n"
"Copy a flac file to memory before playing and copy it into a playback\n"
"pipe.\n"
"\n"
"cptoshm --file=mymusic.flac --shmname=/play.flac\n"
"\n"
"shmcat --shmname=/play.flac | ...\n"
"\n"
  );
}



int main(int argc, char *argv[])
{
    char *infile, *memname;
    int ifd, fd, i, bufsize, optc, overwrite, verbose;
    unsigned int *iptr;
    size_t length, done, rlen;
    struct stat sb;
    char *buf, *mem, *ptr;

    /* read command line options */
    static struct option longoptions[] = {
        {"file", required_argument, 0, 'i' },
        {"shmname", required_argument, 0, 'o' },
        {"buffer-size", required_argument, 0,  'b' },
        {"max-input", required_argument, 0, 'm' },
        {"overwrite", required_argument, 0, 'O' },
        {"verbose", no_argument, 0, 'v' },
        {"version", no_argument, 0, 'V' },
        {"help", no_argument, 0, 'h' },
        {0,         0,                 0,  0 }
    };

    if (argc == 1) {
       usage();
       exit(0);
    }
    /* defaults */
    bufsize = 4194304;
    ifd = 0; /* stdin */
    length = 0;
    overwrite = 1;
    verbose = 0;
    infile = NULL;
    while ((optc = getopt_long(argc, argv, "i:o:b:m:O:vVh",
            longoptions, &optind)) != -1) {
        switch (optc) {
        case 'i':
          infile = optarg;
          if (strcmp(infile, "-")==0)
            break;
          if ((ifd = open(infile, O_RDONLY)) == -1) {
              fprintf(stderr, "Cannot open input file %s.\n", infile);
              exit(2);
          }
          if (fstat(ifd, &sb) == -1) {
              fprintf(stderr, "Cannot stat input file %s.\n", infile);
              exit(4);
          }
          length = sb.st_size;
          break;
        case 'o':
          memname = optarg;
          if ((fd = shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) 
                                                                      == -1){
              fprintf(stderr, "Cannot open memory %s.\n", memname);
              exit(3);
          }
          break;
        case 'b':
          bufsize = atoi(optarg);
          break;
        case 'm':
          length = atoi(optarg);
          break;
        case 'O':
          overwrite = atoi(optarg);
          break;
        case 'v':
          verbose = 1;
          break;
        case 'V':
          fprintf(stderr,
                  "cptoshm (version %s of frankl's stereo utilities)\n",
                  VERSION);
          exit(0);
        default:
          usage();
          exit(0);
        }
    }
    if (ifd == 0) {
        infile = "stdin";
    }
    if (verbose) {
        fprintf(stderr, 
                "cptoshm: input from %s, shared mem is %s, max length %ld\n",
                infile, memname, (long)length);
    }
    if (! (buf = malloc(bufsize)) ) {
        fprintf(stderr, "Cannot allocate buffer of length %ld.\n", 
                        (long)bufsize);
        exit(1);
    }
    if (ftruncate(fd, length) == -1) {
        fprintf(stderr, "Cannot truncate shared memory to %d.", length);
        exit(5);
    }
    mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "Cannot map shared memory.");
        exit(6);
    }
    /* clear memory */
    if (verbose) {
        fprintf(stderr, "cptoshm: clearing memory ... "); 
        fflush(stderr);
    }
    for (done = 0, iptr = (unsigned int*) mem; 
                   done < length/sizeof(int); iptr++, done++)
        *iptr = 2863311530u;  
    for (done = 0, iptr = (unsigned int*) mem; 
                   done < length/sizeof(int); iptr++, done++)
        *iptr = 4294967295u;
    i=overwrite;
    while (i--) {
        for (done = 0, iptr = (unsigned int*) mem; 
                       done < length/sizeof(int); iptr++, done++)
            *iptr = 0;
    }
    if (verbose)
        fprintf(stderr, "cptoshm: done\n");
    /* copy data */
    ptr = mem;
    done = 0;
    while (done < length) {
        rlen = read(ifd, buf, bufsize);
        rlen = (done+rlen > length)? length-done:rlen;
        if (rlen == 0) break;
        /* memcpy(ptr, buf, rlen);  */
        {
           int count=1, *ip, *op, tlen;
           while (count--) {
              for (ip = (int*)buf, op = (int*)ptr, tlen = 0;
                         tlen < rlen/sizeof(int); tlen++, ip++, op++)
                  *op = *ip;
              memcpy( (void*)op, (void*)ip, rlen-tlen*sizeof(int));
           }
        }
        done += rlen;
        ptr += rlen;
    }
    if (done < length) {
      if (ftruncate(fd, done) == -1) {
          fprintf(stderr, "Cannot truncate shared memory to true length %d.", 
                          done);
          exit(7);
      }
    }
    if (verbose) 
        fprintf(stderr, "cptoshm: copied %ld bytes.\n", (long)done);
    exit(0);
}
