/*
bufhrt.c                Copyright frankl 2013-2015

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.
*/


#include "version.h"
#include "net.h"
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "cprefresh.h"

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
          "bufhrt (version %s of frankl's stereo utilities)\nUSAGE:\n",
          VERSION);
  fprintf(stderr,
"\n"
"  bufhrt [options] [--shared <snam1> <snam2> [...]]\n"
"\n"
"  This program reads data from stdin, a file or a network connection and\n"
"  writes it to stdout, a file or the network in precisely timed chunks.\n"
"  To be accurate, the Linux kernel should have the highres-timer enabled\n"
"  (which is the case in most Linux distributions).\n"
"\n"
"  The default input is stdin. Otherwise specify a file or a network \n"
"  connection. Furthermore, the number of bytes to be written per second\n"
"  must be given. In a given number of loops per second the program \n"
"  reads a chunk of data (if needed), sleeps until a specific instant of\n"
"  time and writes a chunk of data after waking up. \n"
"\n"
"  The default output is stdout, otherwise specify a network port or file\n"
"  name.\n"
"\n"
"  USAGE HINTS\n"
"\n"
"  For critical applications (e.g., sending audio data to our player program\n"
"  'playhrt') 'bufhrt' may be started with a high priority for the real\n"
"  time scheduler:  chrt -f 99 bufhrt .....\n"
"  See the documentation of 'playhrt' for more details.\n"
"\n"
"  OPTIONS\n"
"\n"
"  --port-to-write=intval, -p intval\n"
"      the network port number to which data are written instead of stdout.\n"
"\n"
"  --outfile=fname, -o fname\n"
"      write to this file instead of stdout.\n"
"\n"
"  --bytes-per-second=intval, -m intval\n"
"      the number of bytes to be written to the output per second.\n"
"      (Alternatively, in case of audio data, the options --sample-rate and \n"
"      --sample-format can be given instead.)\n"
"\n"
"  --loops-per-second=intval, -n intval\n"
"      the number of loops per second in which this program is reading data,\n"
"      sleeping and then writing a chunk of data. Default is 1000.\n"
"\n"
"  --buffer-size=intval, -b intval\n"
"      the size of the buffer between reading and writing data in bytes.\n"
"      Default is 65536. If reading the input is instable then a larger buffer \n"
"      can be useful. Otherwise, it should be small to fit in memory cache.\n"
"\n"
"  --file=fname, -F fname\n"
"      the name of a file that should be read instead of standard input.\n"
"\n"
"  --host-to-read=hname, -H hname\n"
"      the name or ip-address of a machine. If given, you have to specify\n"
"      --port-to-read as well. In this case data are not read from stdin\n"
"      but from this host and port.\n"
"\n"
"  --port-to-read=intval, -P intval\n"
"      a port number, see --host-to-read.\n"
"\n"
"  --stdin, -S\n"
"      read data from stdin. This is the default.\n"
"\n"
"  --shared <snam1> <snam2> ...\n"
"      input is read from shared memory. The names <snam1>, ..., of the memory \n"
"      chunks must be specified after all other options. This option can be used\n"
"      together with 'writeloop' which writes the data to the memory chunks (see\n"
"      documentation of 'writeloop'). For transfering music data this option may\n"
"      lead to better sound quality than using 'catloop' and a pipe to 'bufhrt'\n"
"      with --stdin input. We have made good experience with using three shared\n"
"      memory chunks whose combined length is a bit smaller than the CPUs \n"
"      L1-cache.\n"
"\n"
"  --input-size=intval, -i intval\n"
"      the number of bytes to be read per loop (when needed). The default\n"
"      is to use the smallest amount needed for the output.\n"
"\n"
"  --extra-bytes-per-second=floatval, -e floatval\n"
"      sometimes the clocks in the sending machine and the receiving\n"
"      machine are not absolutely synchronous. This option allows\n"
"      to specify an amount of extra bytes to be written to the output\n"
"      per second (negativ for fewer and positive for more bytes). \n"
"      The program adjusts the duration of the read-sleep-write rounds.\n"
"\n"
"  --interval, -I\n"
"      use interval mode, typically together with a large --buffer-size.\n"
"      Per interval the buffer is filled, and then written out in a \n"
"      sleep-write loop (without reads during the loop). See below for an \n"
"      example.\n"
"\n"
"  --verbose, -v\n"
"      print some information during startup and operation.\n"
"\n"
"  --version, -V\n"
"      print information about the version of the program and abort.\n"
"\n"
"  --help, -h\n"
"      print this help page and abort.\n"
"\n"
"  EXAMPLES\n"
"\n"
"  Sending stereo audio data (192/32 format) from a filter program to a remote \n"
"  machine, using a small buffer and smallest possible input chunks (here\n"
"  32 bit means 4 byte per channel and sample, so we have 2x4x192000 = 1536000\n"
"  bytes of audio data per second):\n"
"\n"
"  ...(filter)... | chrt -r 99 bufhrt --port-to-write 5888 \\\n"
"                    --bytes-per-second=1536000 --loops-per-second=2000 \\\n"
"                    --buffer-size=8096 \n"
"\n"
"  A network buffer, reading from and writing to network:\n"
"\n"
"  chrt -r 99 bufhrt --host-to-read=myserver --port-to-read=5888 \\\n"
"               --buffer-size=20000 --bytes-per-second=1536000 \\\n"
"               --loops-per-second=2000 --port-to-write=5999\n"
"\n"
"  We use interval mode to copy music files to a hard disk. (Yes, different\n"
"  copies of a music file on the same disk can sound differently . . .):\n"
"  \n"
"  bufhrt --interval --file music.flac --outfile=music_better.flac \\\n"
"         --buffer-size=500000000 --loops-per-second=2000 \\\n"
"         --bytes-per-second=6144000\n"
"\n"
  );
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    int listenfd, connfd, ifd, s, moreinput, optval=1, verbose, rate,
        extrabps, bytesperframe, optc, interval, shared, innetbufsize, 
        outnetbufsize;
    long blen, hlen, ilen, olen, outpersec, loopspersec, nsec, count, wnext,
         badreads, badreadbytes, badwrites, badwritebytes, lcount;
    long long icount, ocount;
    void *buf, *iptr, *optr, *max;
    char *port, *inhost, *inport, *outfile, *infile;
    struct timespec mtime;
    double looperr, extraerr, off;
    /* variables for shared memory input */
    char **fname, *fnames[100], **tmpname, *tmpnames[100], **mem, *mems[100],
         *ptr;
    sem_t **sem, *sems[100], **semw, *semsw[100];
    int fd[100], i, flen, size, c, sz;
    struct stat sb;

    /* read command line options */
    static struct option longoptions[] = {
        {"port-to-write", required_argument,       0,  'p' },
        /* for backward compatibility */
        {"port", required_argument,       0,  'p' },
        {"outfile", required_argument, 0, 'o' },
        {"buffer-size", required_argument,       0,  'b' },
        {"input-size",  required_argument, 0, 'i'},
        {"loops-per-second", required_argument, 0,  'n' },
        {"bytes-per-second", required_argument, 0,  'm' },
        {"sample-rate", required_argument, 0,  's' },
        {"sample-format", required_argument, 0, 'f' },
        {"file", required_argument, 0, 'F' },
        {"host-to-read", required_argument, 0, 'H' },
        {"port-to-read", required_argument, 0, 'P' },
        {"shared", no_argument, 0, 'S' },
        {"extra-bytes-per-second", required_argument, 0, 'e' },
        {"in-net-buffer-size", required_argument, 0, 'K' },
        {"out-net-buffer-size", required_argument, 0, 'L' },
        {"overwrite", required_argument, 0, 'O' }, /* not used, ignored */
        {"interval", no_argument, 0, 'I' },
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
    port = NULL;
    outfile = NULL;
    blen = 65536;
    /* default input is stdin */
    ifd = 0;
    /* default output is stdout */
    connfd = 1;
    ilen = 0;
    loopspersec = 1000;
    outpersec = 0;
    rate = 0;
    bytesperframe = 0;
    inhost = NULL;
    inport = NULL;
    infile = NULL;
    shared = 0;
    interval = 0;
    extrabps = 0;
    innetbufsize = 0;
    outnetbufsize = 0;
    verbose = 0;
    while ((optc = getopt_long(argc, argv, "p:o:b:i:n:m:s:f:F:H:P:e:vVh",
            longoptions, &optind)) != -1) {
        switch (optc) {
        case 'p':
          port = optarg;
          break;
        case 'o':
          outfile = optarg;
          if ((connfd = open(outfile, O_WRONLY | O_CREAT, 00644)) == -1) {
              fprintf(stderr, "Cannot open output file %s.\n   %s\n",
                               outfile, strerror(errno));
              exit(3);
          }
          break;
        case 'b':
          blen = atoi(optarg);
          break;
        case 'i':
          ilen = atoi(optarg);
          break;
        case 'n':
          loopspersec = atoi(optarg);
          break;
        case 'm':
          outpersec = atoi(optarg);
          break;
        case 's':
          rate = atoi(optarg);
          break;
        case 'f':
          if (strcmp(optarg, "S16_LE")==0) {
             bytesperframe = 4;
          } else if (strcmp(optarg, "S24_LE")==0) {
             bytesperframe = 8;
          } else if (strcmp(optarg, "S24_3LE")==0) {
             bytesperframe = 6;
          } else if (strcmp(optarg, "S32_LE")==0) {
             bytesperframe = 8;
          } else {
             fprintf(stderr, "Sample format %s not recognized.\n", optarg);
             exit(1);
          }
          break;
        case 'F':
          infile = optarg;
          if ((ifd = open(infile, O_RDONLY)) == -1) {
              fprintf(stderr, "Cannot open input file %s.\n", infile);
              exit(2);
          }
          break;
        case 'H':
          inhost = optarg;
          break;
        case 'P':
          inport = optarg;
          break;
        case 'S':
          shared = 1;
          break;
        case 'e':
          extrabps = atof(optarg);
          break;
        case 'K':
          innetbufsize = atoi(optarg);
          if (innetbufsize != 0 && innetbufsize < 128)
              innetbufsize = 128;
          break;
        case 'L':
          outnetbufsize = atoi(optarg);
          if (outnetbufsize != 0 && outnetbufsize < 128)
              outnetbufsize = 128;
          break;
        case 'O':
          break;   /* ignored */
        case 'I':
          interval = 1;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'V':
          fprintf(stderr,
                  "bufhrt (version %s of frankl's stereo utilities)\n",
                  VERSION);
          exit(0);
        default:
          usage();
          exit(3);
        }
    }
    /* check some arguments and set some parameters */
    if (outpersec == 0) {
       if (rate != 0 && bytesperframe != 0) {
           outpersec = rate * bytesperframe;
       } else {
           fprintf(stderr, "Specify --bytes-per-second (or rate and format "
                           "of audio data).\n");
           exit(5);
       }
    }
    if (inhost != NULL && inport != NULL) {
       ifd = fd_net(inhost, inport);
        if (innetbufsize != 0  &&
            setsockopt(ifd, SOL_SOCKET, SO_RCVBUF, (void*)&innetbufsize, sizeof(int)) < 0) {
                fprintf(stderr, "playhrt: cannot set buffer size for network socket to %d.\n",
                        innetbufsize);
                exit(23);
        }
    }
    if (verbose) {
       fprintf(stderr, "bufhrt: Writing %ld bytes per second to ", outpersec);
       if (port != NULL)
          fprintf(stderr, "port %s.\n", port);
       else if (connfd == 1)
          fprintf(stderr, "stdout.\n");
       else
          fprintf(stderr, " file %s.\n", outfile);
       fprintf(stderr, "bufhrt: Input from ");
       if (shared)
          fprintf(stderr, "shared memory");
       else if (ifd == 0)
          fprintf(stderr, "stdin");
       else if (inhost != NULL)
          fprintf (stderr, "host %s (port %s)", inhost, inport);
       else
          fprintf(stderr, "file %s", infile);
       fprintf(stderr, ", output in %ld loops per second.\n", loopspersec);
    }

    extraerr = 1.0*outpersec/(outpersec+extrabps);
    nsec = (int) (1000000000*extraerr/loopspersec);
    olen = outpersec/loopspersec;
    if (olen <= 0)
        olen = 1;
    if (interval) {
        if (ilen == 0)
            ilen = 16384;
    }
    else if (ilen < olen) {
        ilen = olen + 2;
        if (olen*loopspersec == outpersec)
            ilen = olen;
        else
            ilen = olen + 1;
        if (verbose) {
           fprintf(stderr, "bufhrt: Choosing %ld as length of input chunks.\n", ilen);
           fflush(stderr);
        }
    }
    if (blen < 3*(ilen+olen))
        blen = 3*(ilen+olen);
    hlen = blen/2;
    if (olen*loopspersec == outpersec)
        looperr = 0.0;
    else
        looperr = (1.0*outpersec)/loopspersec - 1.0*olen;
    moreinput = 1;
    icount = 0;
    ocount = 0;

    /* we want buf % 8 = 0 */
    if (! (buf = malloc(blen+ilen+2*olen+8)) ) {
        fprintf(stderr, "Cannot allocate buffer of length %ld.\n",
                blen+ilen+olen);
        exit(6);
    }
    while (((uintptr_t)buf % 8) != 0) buf++;
    buf = buf + 2*olen;
    max = buf + blen;
    iptr = buf;
    optr = buf;

    /* outgoing socket */
    if (port != 0) {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd < 0) {
            fprintf(stderr, "Cannot create outgoing socket.\n");
            exit(9);
        }
        if (setsockopt(listenfd,
                       SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int)) == -1)
        {
            fprintf(stderr, "Cannot set REUSEADDR.\n");
            exit(10);
        }
        if (outnetbufsize != 0 && setsockopt(listenfd,
                       SOL_SOCKET,SO_SNDBUF,&outnetbufsize,sizeof(int)) == -1)
        {
            fprintf(stderr, "Cannot set outgoing network buffer to %d.\n",
                    outnetbufsize);
            exit(30);
        }
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(atoi(port));
        if (bind(listenfd, (struct sockaddr*)&serv_addr,
                                              sizeof(serv_addr)) == -1) {
            fprintf(stderr, "Cannot bind outgoing socket.\n");
            exit(11);
        }
        listen(listenfd, 1);
        if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {
            fprintf(stderr, "Cannot accept outgoing connection.\n");
            exit(12);
        }
    }
    /* shared memory input */
    if (shared) {
      size = 0;
      badwrites = 0;
      badwritebytes = 0;
      /* prepare shared memory */
      for (i=optind; i < argc; i++) {
         if (i>100) {
            fprintf(stderr, "Too many filenames.");
            exit(6);
         }
         fnames[i-optind] = argv[i];
         if (shared) {
             /* open semaphore with same name as memory */
             if ((sems[i-optind] = sem_open(fnames[i-optind], O_RDWR))
                                                          == SEM_FAILED) {
                 fprintf(stderr, "Cannot open semaphore.");
                 exit(20);
             }
             /* also semaphore for write lock */
             tmpnames[i-optind] = (char*)malloc(strlen(argv[i])+5);
             strncpy(tmpnames[i-optind], fnames[i-optind], strlen(argv[i]));
             strncat(tmpnames[i-optind], ".TMP", 4);
             if ((semsw[i-optind] = sem_open(tmpnames[i-optind], O_RDWR))
                                                           == SEM_FAILED) {
                 fprintf(stderr, "Cannot open write semaphore.");
                 exit(21);
             }
             /* open shared memory */
             if ((fd[i-optind] = shm_open(fnames[i-optind],
                                 O_RDWR, S_IRUSR | S_IWUSR)) == -1){
                 fprintf(stderr, "Cannot open shared memory %s.\n", fnames[i-optind]);
                 exit(22);
             }
             if (size == 0) { /* find size of shared memory chunks */
                 if (fstat(fd[i-optind], &sb) == -1) {
                     fprintf(stderr, "Cannot stat shared memory %s.\n", fnames[i-optind]);
                     exit(24);
                 }
                 size = sb.st_size - sizeof(int);
             }
             /* map the memory (will be on page boundary, so 0 mod 8) */
             mems[i-optind] = mmap(NULL, sizeof(int)+size,
                             PROT_WRITE | PROT_READ, MAP_SHARED, fd[i-optind], 0);
             if (mems[i-optind] == MAP_FAILED) {
                 fprintf(stderr, "Cannot map shared memory.");
                 exit(24);
             }
         }
      }
      fnames[argc-optind] = NULL;
      fname = fnames;
      tmpname = tmpnames;
      mem = mems;
      sem = sems;
      semw = semsw;
      clock_gettime(CLOCK_MONOTONIC, &mtime);
      lcount = 0;
      off = looperr;
      i = 0; /* counter to update mtime */
      while (1) {
         i++;
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
         icount += flen;
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
         /* write shared memory content to output */
         ptr = *mem + sizeof(int);
         sz = 0;
         if (i == 100)
           clock_gettime(CLOCK_MONOTONIC, &mtime);
         while (sz < flen) {
             mtime.tv_nsec += nsec;
             if (mtime.tv_nsec > 999999999) {
               mtime.tv_nsec -= 1000000000;
               mtime.tv_sec++;
             }
             if (flen - sz <= olen) {
                 c = flen - sz;
                 off += (olen-c);
             } else {
                 c = olen;
                 if (off >= 1.0) {
                    off -= 1.0;
                    c++;
                 }
             }
             refreshmem((char*)ptr, c);
             refreshmem((char*)ptr, c);
             refreshmem((char*)ptr, c);
             while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                                                &mtime, NULL)
                    != 0) ;
             /* write a chunk, this comes first after waking from sleep */
             s = write(connfd, ptr, c);
             if (s < 0) {
                 fprintf(stderr, "bufhrt (from shared): Write error: %s\n", strerror(errno));
                 exit(15);
             }
             if (s < c) {
                 badwrites++;
                 badwritebytes += (c-s);
                 off += (c-s);
                 fprintf(stderr, "*%ld", (long)(c-s)); fflush(stderr);
             }
             ocount += c;
             ptr += c;
             sz += c;
             lcount++;
             off += looperr;
         }
         /* mark as writable */
         sem_post(*semw);
         fname++;
         tmpname++;
         mem++;
         sem++;
         semw++;
      }
      close(connfd);
      shutdown(listenfd, SHUT_RDWR);
      close(listenfd);
      if (verbose)
        fprintf(stderr, "bufhrt: Loops: %ld, total bytes: %lld in (shared mem) %lld out.\n"
                        "bufhrt: bad writes: %ld (%ld bytes)\n",
                        lcount, icount, ocount, badwrites, badwritebytes);
      return 0;
    }

    /* interval mode */
    if (interval) {
       count = 0;
       while (moreinput) {
          count++;
          /* fill buffer */
          memclean(buf, 2*hlen);
          for (iptr = buf; iptr < buf + 2*hlen - ilen; ) {
              s = read(ifd, iptr, ilen);
              if (s < 0) {
                  fprintf(stderr, "Read error.\n");
                  exit(18);
              }
              icount += s;
              if (s == 0) {
                  moreinput = 0;
                  break;
              }
              iptr += s;
          }

          /* write out */
          optr = buf;
          wnext = (iptr - optr <= olen) ? (iptr - optr) : olen;
          clock_gettime(CLOCK_MONOTONIC, &mtime);
          for (lcount=0, off=looperr; optr < iptr; lcount++, off+=looperr) {
              /* once cache is filled and other side is reading we reset time */
              if (lcount == 50) clock_gettime(CLOCK_MONOTONIC, &mtime);
              mtime.tv_nsec += nsec;
              if (mtime.tv_nsec > 999999999) {
                mtime.tv_nsec -= 1000000000;
                mtime.tv_sec++;
              }
              refreshmem((char*)optr, wnext);
              refreshmem((char*)optr, wnext);
              refreshmem((char*)optr, wnext);
              while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                                                 &mtime, NULL)
                     != 0) ;
              /* write a chunk, this comes first after waking from sleep */
              s = write(connfd, optr, wnext);
              if (s < 0) {
                  fprintf(stderr, "Write error.\n");
                  exit(15);
              }
              ocount += s;
              optr += s;
              wnext = olen + wnext - s;
              if (off >= 1.0) {
                 off -= 1.0;
                 wnext++;
              }
              if (wnext >= 2*olen) {
                 fprintf(stderr, "Underrun by %ld (%ld sec %ld nsec).\n",
                           wnext - 2*olen, mtime.tv_sec, mtime.tv_nsec);
                 wnext = 2*olen-1;
              }
              s = iptr - optr;
              if (s <= wnext) {
                  wnext = s;
              }
          }
       }

       close(connfd);
       shutdown(listenfd, SHUT_RDWR);
       close(listenfd);
       close(ifd);
       if (verbose)
           fprintf(stderr, "bufhrt: Intervals: %ld, total bytes: %lld in %lld out.\n",
                            count, icount, ocount);
       exit(0);
    }

    /* default mode, no shared memory input and no interval mode */
    /* fill at least half buffer */
    memclean(buf, 2*hlen);
    for (; iptr < buf + 2*hlen - ilen; ) {
        s = read(ifd, iptr, ilen);
        if (s < 0) {
            fprintf(stderr, "Read error.\n");
            exit(13);
        }
        icount += s;
        if (s == 0) {
            moreinput = 0;
            break;
        }
        iptr += s;
    }
    if (iptr - optr < olen)
        wnext = iptr-optr;
    else
        wnext = olen;

    if (clock_gettime(CLOCK_MONOTONIC, &mtime) < 0) {
        fprintf(stderr, "Cannot get monotonic clock.\n");
        exit(14);
    }
    if (verbose) {
        fprintf(stderr, "bufhrt: Starting at %ld sec %ld nsec \n",
                                           mtime.tv_sec, mtime.tv_nsec);
        fprintf(stderr,
                "bufhrt:    insize %ld, outsize %ld, buflen %ld, interval %ld nsec\n",
                                     ilen, olen, blen+ilen+2*olen, nsec);
    }

    /* main loop */
    badreads = 0;
    badwrites = 0;
    badreadbytes = 0;
    badwritebytes = 0;
    for (count=1, off=looperr; 1; count++, off+=looperr) {
        /* once cache is filled and other side is reading we reset time */
        if (count == 500) clock_gettime(CLOCK_MONOTONIC, &mtime);
        mtime.tv_nsec += nsec;
        if (mtime.tv_nsec > 999999999) {
          mtime.tv_nsec -= 1000000000;
          mtime.tv_sec++;
        }
        refreshmem((char*)optr, wnext);
        refreshmem((char*)optr, wnext);
        refreshmem((char*)optr, wnext);
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mtime, NULL)
               != 0) ;
        /* write a chunk, this comes first after waking from sleep */
        s = write(connfd, optr, wnext);
        if (s < 0) {
            fprintf(stderr, "Write error.\n");
            exit(15);
        }
        if (s < wnext) {
            badwrites++;
            badwritebytes += (wnext-s);
        }
        ocount += s;
        optr += s;
        wnext = olen + wnext - s;
        if (off >= 1.0) {
           off -= 1.0;
           wnext++;
        }
        if (wnext >= 2*olen) {
           fprintf(stderr, "Underrun by %ld (%ld sec %ld nsec).\n",
                     wnext - 2*olen, mtime.tv_sec, mtime.tv_nsec);
           wnext = 2*olen-1;
        }
        s = (iptr >= optr ? iptr - optr : iptr+blen-optr);
        if (s <= wnext) {
            wnext = s;
        }
        if (optr+wnext >= max) {
            optr -= blen;
        }
        /* read if buffer not half filled */
        if (moreinput && (iptr > optr ? iptr-optr : iptr+blen-optr) < hlen) {
            memclean(iptr, ilen);
            s = read(ifd, iptr, ilen);
            if (s < 0) {
                fprintf(stderr, "Read error.\n");
                exit(16);
            }
            if (s < ilen) {
                badreads++;
                badreadbytes += (ilen-s);
            }
            icount += s;
            iptr += s;
            if (iptr >= max) {
                memcpy(buf-2*olen, max-2*olen, iptr-max+2*olen);
                iptr -= blen;
            }
            if (s == 0) { /* input complete */
                moreinput = 0;
            }
        }
        if (wnext == 0)
            break;    /* done */
    }
    close(connfd);
    shutdown(listenfd, SHUT_RDWR);
    close(listenfd);
    close(ifd);
    if (verbose)
        fprintf(stderr, "bufhrt: Loops: %ld, total bytes: %lld in %lld out.\n"
                        "bufhrt: Bad reads/bytes %ld/%ld and writes/bytes %ld/%ld.\n",
                        count, icount, ocount, badreads, badreadbytes,
                        badwrites, badwritebytes);
    return 0;
}

