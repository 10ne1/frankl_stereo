/*
catloop.c                Copyright frankl 2013-2015

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.
*/

#define _GNU_SOURCE
#include "version.h"
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
          "catloop (version %s of frankl's stereo utilities)\nUSAGE:\n",
          VERSION);
  fprintf(stderr,

"  catloop --block-size=<bsize> <file1> <file2> [<file3>..]\n"
"\n"
"  This program 'catloop' reads data cyclically from files <file1>, ... \n"
"  and writes the content to stdout. \n"
"\n"
"  USAGE HINTS\n"
"  \n"
"  This can be used together with 'writeloop' which does the converse\n"
"  (reading from stdin and writing to files cyclically).\n"
"\n"
"  The given files should be on a ramdisk. writeloop/catloop provide\n"
"  a buffer for data read from a hard disk or from the network.\n"
"\n"
"  Alternatively, shared memory can be used instead of files via\n"
"  the --shared option. In this case the convention is to use filenames\n"
"  with empty path but starting with a slash.\n"
"  \n"
"  OPTIONS\n"
"\n"
"  --block-size=intval, -b intval\n"
"      the size in bytes of the data chunks written to stdout.\n"
"      Default is 1024 bytes.\n"
"\n"
"  --shared, -s\n"
"      use named shared memory instead of files. For large amounts of shared \n"
"      memory you may need to enlarge '/proc/sys/kernel/shmmax' directly or\n"
"      via sysctl.\n"
"\n"
"  --verbose, -v\n"
"    print some information during startup.\n"
"\n"
"  --version, -V\n"
"      print information about the version of the program and abort.\n"
"\n"
"  --help, -h\n"
"      print this help page and abort.\n"
"\n"
"  EXAMPLE\n"
"  \n"
"  If you want to play an audio file (CD format) on a hard disk or on a \n"
"  network file system, you can use writeloop and catloop as a buffer:\n"
"\n"
"       sox /my/disk/cd.flac -t raw | writeloop --block-size=4000 \\\n"
"                --file-size=20000 /ramdisk/aa /ramdisk/bb /ramdisk/cc &\n"
"\n"
"  and \n"
"       catloop --block-size=1000 /ramdisk/aa /ramdisk/bb /ramdisk/cc | \\\n"
"             aplay -t raw -f S16_LE -c 2 -r44100 -D \"hw:0,0\" \n"
"\n"
"  Or similarly, using less system resources, with shared memory:\n"
"\n"
"       sox /my/disk/cd.flac -t raw | writeloop --block-size=4000 \\\n"
"                --file-size=20000 --shared /aa /bb /cc &\n"
"\n"
"  and \n"
"       catloop --block-size=1000 --shared /aa /bb /cc | \\\n"
"             aplay -t raw -f S16_LE -c 2 -r44100 -D \"hw:0,0\" \n"
"\n"
"     \n"
"  In experiments I found that audio playback was improved compared to a \n"
"  direct playing. For larger file sizes (a few megabytes) the effect was\n"
"  similar to copying the file into RAM and playing that file.\n"
"  But even better was the effect with small file sizes (a few kilobytes)\n"
"  such that all files fit into the processor cache. \n"
"\n"
);
}


int main(int argc, char *argv[])
{
    char **fname, *fnames[100], **tmpname, *tmpnames[100], **mem, *mems[100],
         *ptr;
    sem_t **sem, *sems[100], **semw, *semsw[100];
    void * buf;
    int optc, infile, fd[100], i, blocksize, size, flen, sz, 
        ret, shared, c, verbose;
    struct stat sb;


    /* read command line options */
    static struct option longoptions[] = {
        {"block-size", required_argument, 0,  'b' },
        {"shared", no_argument, 0, 's' },
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
    blocksize = 1024;
    shared = 0;
    verbose = 0;
    size = 0;
    while ((optc = getopt_long(argc, argv, "b:Vh",
            longoptions, &optind)) != -1) {
        switch (optc) {
        case 'b':
          blocksize = atoi(optarg);
          break;
        case 's':
          shared = 1;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'V':
          fprintf(stderr,
                  "catloop (version %s of frankl's stereo utilities)\n",
                  VERSION);
          exit(0);
        default:
          usage();
          exit(2);
        }
    }
    /* args:   fname  blocksize  */
    if (argc-optind < 1) {
       fprintf(stderr, "catloop: Need at least two files fnam1 fnam2 [...].\n");
       exit(3);
    }
    buf = malloc(blocksize);
    if (! buf) {
       fprintf(stderr, "catloop: Cannot allocate buffer.\n");
       exit(4);
    }
    for (i=optind; i < argc; i++) {
       if (i>100) {
          fprintf(stderr, "catloop: Too many filenames.\n");
          exit(6);
       }
       fnames[i-optind] = argv[i];
       if (shared) {
           /* open semaphore with same name as memory */
           if ((sems[i-optind] = sem_open(fnames[i-optind], O_RDWR))
                                                        == SEM_FAILED) {
               fprintf(stderr, "catloop: Cannot open semaphore: %s\n", strerror(errno));
               exit(20);
           }
           /* also semaphore for write lock */
           tmpnames[i-optind] = (char*)malloc(strlen(argv[i])+5);
           strncpy(tmpnames[i-optind], fnames[i-optind], strlen(argv[i]));
           strncat(tmpnames[i-optind], ".TMP", 4);
           if ((semsw[i-optind] = sem_open(tmpnames[i-optind], O_RDWR))
                                                         == SEM_FAILED) {
               fprintf(stderr, "catloop: Cannot open write semaphore: %s\n", strerror(errno));
               exit(21);
           }
           /* open shared memory */
           if ((fd[i-optind] = shm_open(fnames[i-optind],
                               O_RDONLY, S_IRUSR | S_IWUSR)) == -1){
               fprintf(stderr, "catloop: Cannot open shared memory %s.\n", fnames[i-optind]);
               exit(22);
           }
           if (size == 0) { /* find size of shared memory chunks */
               if (fstat(fd[i-optind], &sb) == -1) {
                   fprintf(stderr, "catloop: Cannot stat shared memory %s.\n", fnames[i-optind]);
                   exit(24);
               }
               size = sb.st_size - sizeof(int);
           }
           /* map the memory */
           mems[i-optind] = mmap(NULL, sizeof(int)+size,
                           PROT_READ, MAP_SHARED, fd[i-optind], 0);
           if (mems[i-optind] == MAP_FAILED) {
               fprintf(stderr, "catloop: Cannot map shared memory.");
               exit(24);
           }
       }
    }
    fnames[argc-optind] = NULL;
    fname = fnames;
    if (shared) {
        if (verbose)
          fprintf(stderr, "catloop: Reading from shared memory.\n"); 
        tmpname = tmpnames;
        mem = mems;
        sem = sems;
        semw = semsw;
        while (1) {
           if (*fname == NULL) {
              fname = fnames;
              tmpname = tmpnames;
              mem = mems;
              sem = sems;
              semw = semsw;
           }
           /* get lock */
           sem_wait(*sem);
           /* find length of relevant memory chunk */
           flen = *((int*)(*mem));
           if (flen == 0) {
               /* done, unlink semaphores and shared memory */
               fname = fnames;
               tmpname = tmpnames;
               while (*fname != NULL) {
                   shm_unlink(*fname);
                   sem_unlink(*fname);
                   sem_unlink(*tmpname);
                   fname++;
                   tmpname++;
               }
               exit(0);
           }
           /* write shared memory content to stdout */
           ptr = *mem + sizeof(int);
           sz = 0;
           while (sz < flen) {
               if (flen - sz <= blocksize)
                   c = flen - sz;
               else
                   c = blocksize;
               ret = write(1, ptr, c);
               if (ret == -1) {
                  fprintf(stderr, "catloop: Write error: %s.\n", strerror(errno));
                  exit(31);
               }
               if (ret < c) {
                  fprintf(stderr, "catloop: Could not write block.\n");
                  exit(32);
               }
               ptr += c;
               sz += c;
           }
           /* mark as writable */
           sem_post(*semw);
           fname++;
           tmpname++;
           mem++;
           sem++;
           semw++;
        }
    } else {
        if (verbose)
           fprintf(stderr, "catloop: Reading from files.\n"); 
        while (1) {
           if (*fname == NULL)
              fname = fnames;
           while (access(*fname, R_OK) != 0)
             usleep(500);
           infile = open(*fname, O_RDONLY|O_NOATIME);
           if (!infile) {
              fprintf(stderr, "catloop: Cannot open for reading: %s.\n", *fname);
              exit(2);
           }
           c = read(infile, buf, blocksize);
           if (c == 0) {
              /* empty file means quit */
              close(infile);
              unlink(*fname);
              exit(0);
           }
           while (c > 0) {
              ret = write(1, buf, c);
              if (ret == -1) {
                  fprintf(stderr, "catloop: Write error: %s.\n", strerror(errno));
                  exit(3);
              }
              if (ret < c) {
                  fprintf(stderr, "catloop: Could not write full buffer.\n");
                  exit(4);
              }
              c = read(infile, buf, blocksize);
           }
           close(infile);
           unlink(*fname);
           fname++;
        }
    }
}

