/*
bufhrt.c                Copyright frankl 2013-2014

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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
"\n"
"  bufhrt [options] \n"
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
"  --port=intval, -p intval\n"
"      the network port number to which data are written instead of stdout.\n"
"\n"
"  --outfile=fname, -o fname\n"
"      write to this file instead of stdout.\n"
"\n"
"  --bytes-per-second=intval, -m intval\n"
"      the number of bytes to be written to the output per second.\n"
"      (Alternatively, in case of audio data the options --sample-rate and \n"
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
"      the name or ip-address of a machine. If given, you need to specify\n"
"      --port-to-read as well. In this case data are not read from stdin\n"
"      but from this host and port.\n"
"\n"
"  --port-to-read=intval, -P intval\n"
"      a port number, see --host-to-read.\n"
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
"      The program adjusts the duration of the read-sleep-write\n"
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
"  Sending audio data (192/32 format) from a filter program to a remote \n"
"  machine, using a small buffer and smallest possible input chunks:\n"
"\n"
"  ...(filter)... | chrt -r 99 bufhrt --port 5888 \\\n"
"                    --bytes-per-second=1536000 --loops-per-second=2000 \\\n"
"                    --buffer-size=8096 \n"
"\n"
"  A network buffer, reading from and writing to network:\n"
"\n"
"  chrt -r 99 bufhrt --host-to-read=myserver --port-to-read=5888 \\\n"
"               --buffer-size=20000 --bytes-per-second=1536000 \\\n"
"               --loops-per-second=2000 --port=5999\n"
"\n"
  );
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    int listenfd, connfd, ifd, s, moreinput, optval=1, verbose, rate,
        extrabps, bytesperframe, optc, interval, overwrite;
    uint *uptr;
    long blen, hlen, ilen, olen, outpersec, loopspersec, nsec, count, wnext,
         badreads, badreadbytes, badwrites, badwritebytes, lcount;
    long long icount, ocount;
    void *buf, *wbuf, *wbuf2, *iptr, *optr, *max;
    char *port, *inhost, *inport, *outfile, *infile;
    struct timespec mtime;
    double looperr, extraerr, off;

    /* read command line options */
    static struct option longoptions[] = {
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
        {"extra-bytes-per-second", required_argument, 0, 'e' },
        {"overwrite", required_argument, 0, 'O' },
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
    overwrite = 0;
    interval = 0;
    extrabps = 0;
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
        case 'e':
          extrabps = atof(optarg);
          break;
        case 'O':
          overwrite = atoi(optarg);
          break;
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
    }
    if (verbose) {
       fprintf(stderr, "Writing %ld bytes per second to ", outpersec);
       if (port != NULL)
          fprintf(stderr, "port %s.\n", port);
       else if (connfd == 1)
          fprintf(stderr, "stdout.\n");
       else 
          fprintf(stderr, " file %s.\n", outfile);
       fprintf(stderr, "Input from ");
       if (ifd == 0)
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
           fprintf(stderr, "Choosing %ld as length of input chunks.\n", ilen);
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

    if (! (buf = malloc(blen+ilen+2*olen)) ) {
        fprintf(stderr, "Cannot allocate buffer of length %ld.\n",
                blen+ilen+olen);
        exit(6);
    }
    buf = buf + 2*olen;
    max = buf + blen;
    iptr = buf;
    optr = buf;
    if (! (wbuf = malloc(2*olen)) ) {
        fprintf(stderr, "Cannot allocate buffer of length %ld.\n",
                blen+ilen+olen);
        exit(7);
    }
    if (! (wbuf2 = malloc(2*olen)) ) {
        fprintf(stderr, "Cannot allocate another buffer of length %ld.\n",
                blen+ilen+olen);
        exit(8);
    }

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
    
    /* interval mode */
    if (interval) {
       count = 0;
       while (moreinput) {
          count++;
          /* fill buffer */
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
          wnext = olen;
          memcpy(wbuf, optr, wnext);
          clock_gettime(CLOCK_MONOTONIC, &mtime);
          for (lcount=0, off=looperr; optr < iptr; lcount++, off+=looperr) {
              /* once cache is filled and other side is reading we reset time */
              /*if (lcount == 50) clock_gettime(CLOCK_MONOTONIC, &mtime);*/
              mtime.tv_nsec += nsec;
              if (mtime.tv_nsec > 999999999) {
                mtime.tv_nsec -= 1000000000;
                mtime.tv_sec++;
              }
              while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, 
                                                                 &mtime, NULL)
                     != 0) ;
              /* write a chunk, this comes first after waking from sleep */
              s = write(connfd, wbuf, wnext);
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
              /* copy data for next write */
              /* data are always lying in same position (hopefully processor
                 cache)   */
              memcpy(wbuf, optr, wnext);
              memcpy(wbuf2, wbuf, wnext);
              /* not sure if this improves performance */
              for (s = 0; s < overwrite; s++) 
                  memcpy(wbuf2, wbuf, wnext);
          }
       }

       close(connfd);
       shutdown(listenfd, SHUT_RDWR);
       close(listenfd);
       close(ifd);
       if (verbose)
           fprintf(stderr, "Intervals: %ld, total bytes: %lld in %lld out.\n",
                            count, icount, ocount);
       exit(0);
    }

    /* fill at least half buffer */
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
        fprintf(stderr, "Starting at %ld sec %ld nsec \n",
                                           mtime.tv_sec, mtime.tv_nsec);
        fprintf(stderr,
                "   insize %ld, outsize %ld, buflen %ld, interval %ld nsec\n",
                                     ilen, olen, blen+ilen+2*olen, nsec);
    }

    /* main loop */
    badreads = 0;
    badwrites = 0;
    badreadbytes = 0;
    badwritebytes = 0;
    memcpy(wbuf, optr, wnext);
    for (count=1, off=looperr; 1; count++, off+=looperr) {
        /* once cache is filled and other side is reading we reset time */
        if (count == 500) clock_gettime(CLOCK_MONOTONIC, &mtime);
        mtime.tv_nsec += nsec;
        if (mtime.tv_nsec > 999999999) {
          mtime.tv_nsec -= 1000000000;
          mtime.tv_sec++;
        }
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mtime, NULL)
               != 0) ;
        /* write a chunk, this comes first after waking from sleep */
        s = write(connfd, wbuf, wnext);
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
        /* copy data for next write */
        /* data are always lying in same position (hopefully processor
           cache)   */
        for (s=0, uptr=(uint*)wbuf; s < wnext/sizeof(uint)+1; s++)
             *uptr++ = 2863311530u;
        for (s=0, uptr=(uint*)wbuf; s < wnext/sizeof(uint)+1; s++)
             *uptr++ = 4294967295u;
        for (s=0, uptr=(uint*)wbuf; s < wnext/sizeof(uint)+1; s++)
             *uptr++ = 0;
        memcpy(wbuf2, optr, wnext);
        memcpy(wbuf, wbuf2, wnext);
        /* not sure if this improves performance */
        for (s = 0; s < overwrite; s++) 
            memcpy(wbuf, wbuf2, wnext);
    }
    close(connfd);
    shutdown(listenfd, SHUT_RDWR);
    close(listenfd);
    close(ifd);
    if (verbose)
        fprintf(stderr, "Loops: %ld, total bytes: %lld in %lld out.\n"
                        "Bad reads/bytes %ld/%ld and writes/bytes %ld/%ld.\n",
                        count, icount, ocount, badreads, badreadbytes,
                        badwrites, badwritebytes);
    return 0;
}
