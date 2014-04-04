/*
catloop.c                Copyright frankl 2013-2014

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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* help page */
/* vim hint to remove resp. add quotes: 
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
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
"  \n"
"  OPTIONS\n"
"\n"
"  --block-size=intval, -b intval\n"
"      the size in bytes of the data chunks written to stdout.\n"
"      Default is 1000 bytes.\n"
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
    char **fname, *fnames[100];
    void * buf;
    int optc, infile, i, blocksize, ret, c;
  
    /* read command line options */
    static struct option longoptions[] = {
        {"block-size", required_argument, 0,  'b' },
        {"version", no_argument, 0, 'V' },
        {"help", no_argument, 0, 'h' },
        {0,         0,                 0,  0 }
    };
    
    if (argc == 1) {
       usage();
       exit(0);
    }
    /* defaults */
    blocksize = 2000;
    while ((optc = getopt_long(argc, argv, "b:Vh",  
            longoptions, &optind)) != -1) {
        switch (optc) {
        case 'b':
          blocksize = atoi(optarg);
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
       fprintf(stderr, "catloop: need at least two files fnam1 fnam2 [...]\n");
       exit(3);
    }
    buf = malloc(blocksize);
    if (! buf) {
       fprintf(stderr, "Cannot allocate buffer.\n");
       exit(4);
    }
    for (i=optind; i < argc; i++)
       fnames[i-optind] = argv[i];
    fnames[argc-optind] = NULL;
    fname = fnames;
    while (1) {
       if (*fname == NULL)
          fname = fnames;
       while (access(*fname, R_OK) != 0)
         usleep(500);
       infile = open(*fname, O_RDONLY|O_NOATIME);
       if (!infile) {
          fprintf(stderr, "Cannot open for reading: %s\n", *fname);
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
              fprintf(stderr, "write error: %s\n", strerror(errno));
              exit(3);
          }
          if (ret < c) {
              fprintf(stderr, "Could not write full buffer.\n");
              exit(4);
          }
          c = read(infile, buf, blocksize);
       }
       close(infile);
       unlink(*fname);
       fname++;
    }
}

