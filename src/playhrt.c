/*
playhrt.c                Copyright frankl 2013-2016

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.
*/

#include "version.h"
#include "net.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <alsa/asoundlib.h>
#include "cprefresh.h"

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
          "playhrt (version %s of frankl's stereo utilities",
          VERSION);
#ifdef ALSANC
  fprintf(stderr, ", with alsa-lib patch");
#endif
  fprintf(stderr, ")\nUSAGE:\n");
  fprintf(stderr,
"\n"
"  playhrt [options] \n"
"\n"
"  This program reads raw(!) stereo audio data from stdin, a file or the \n"
"  network and plays it on a local (ALSA) sound device. \n"
"\n"
"  The program repeats in a given number of loops per second: reading\n"
"  a chunk of input data, preparing data for the audio driver, then it\n"
"  sleeps until a specific instant of time and after wakeup it hands data\n"
"  to the audio driver. In contrast to other player programs this is done\n"
"  with a very precise timing such that no buffers underrun or overrun an\n"
"  no reading or writing of data is blocking. Furthermore, the data are\n"
"  refreshed in RAM directly before turning them to the audio driver.\n"
"\n"
"  The Linux kernel needs the highres-timer functionality enabled (on most\n"
"  systems this is the case).\n"
"\n"
"  The program works in two modes, one with an internal buffer (of\n"
"  configurable size) and one which writes input data directly to the\n"
"  memory of the audio driver (see the --mmap option below). The second\n"
"  mode is recommended.\n"
"\n"
"  USAGE HINTS\n"
"  \n"
"  It is recommended to give this program a high priority and not to run\n"
"  too many other things on the same computer during playback. A high\n"
"  priority can be specified with the 'chrt' command:\n"
"\n"
"  chrt -f 99 playhrt .....\n"
"\n"
"  (Depending on the configuration of your computer you may need root\n"
"  privileges for this, in that case use 'sudo chrt -f 99 playhrt ....' \n"
"  or give 'chrt' setuid permissions.)\n"
"\n"
"  While running this program the computer should run as few other things\n"
"  as possible. In particular we recommend to generate the input data\n"
"  on a different computer and to send them via the network to 'playhrt'\n"
"  using the program 'bufhrt' which is also contained in this package. \n"
"  \n"
"  OPTIONS\n"
"\n"
"  --host=hostname, -r hostname\n"
"      the host from which to receive the data , given by name or\n"
"      ip-address.\n"
"\n"
"  --port=portnumber, -p portnumber\n"
"      the port number on the remote host from which to receive data.\n"
"\n"
"  --stdin, -S\n"
"      read data from stdin (instead of --host and --port).\n"
"\n"
"  --device=alsaname, -d alsaname\n"
"      the name of the sound device. A typical name is 'hw:0,0', maybe\n"
"      use 'aplay -l' to find out the correct numbers. It is recommended\n"
"      to use the hardware devices 'hw:...' if possible.\n"
"\n"
"  --sample-rate=intval, -s intval\n"
"      the sample rate of the audio data. Default is 44100 as on CDs.\n"
"\n"
"  --sample-format=formatstring, -f formatstring\n"
"      the format of the samples in the audio data. Currently recognised\n"
"      are 'S16_LE' (the sample format on CDs), 'S24_LE' \n"
"      (signed integer data with 24 bits packed into 32 bit words, used by\n"
"      many DACs), 'S24_3LE' (also 24 bit integers but only using 3 bytes\n"
"      per sample), 'S32_LE' (true 32 bit signed integer samples).\n"
"      Default is 'S16_LE'.\n"
"\n"
"  --number-channels=intval, -k intval\n"
"      the number of channels in the (interleaved) audio stream. The \n"
"      default is 2 (stereo).\n"
"\n"
"  --loops-per-second=intval, -n intval\n"
"      the number of loops per second in which 'playhrt' reads some\n"
"      data from the network into a buffer, sleeps until a precise\n"
"      moment and then writes a chunk of data to the sound device. \n"
"      Typical values would be 1000 or 2000. Default is 1000.\n"
"\n"
"  --non-blocking-write, -N\n"
"      write data to sound device in a non-blocking fashion. This can\n"
"      improve sound quality, but the timing must be very precise.\n"
"\n"
"  --mmap, -M\n"
"      write data directly to the sound device via an mmap'ed memory\n"
"      area. In this mode --buffer-size and --input-size are ignored.\n"
"      If you hear clicks enlarge the --hw-buffer. This mode is \n"
"      recommended.\n"
"\n"
"  --buffer-size=intval, -b intval\n"
"      the size of the internal buffer for incoming data in bytes.\n"
"      It can make sense to play around with this value, a larger\n"
"      value (say up to some or many megabytes) can be useful if the \n"
"      network is busy. Smaller values (a few kilobytes) may enable\n"
"      'playhrt' to mainly operate in memory cache and improve sound\n"
"      quality. Default is 65536 bytes. You may specify 0 or some small\n"
"      number, in which case 'playhrt' will compute and use a minimal\n"
"      amount of memory it needs, depending on the other parameters.\n"
"\n"
"  --input-size=intval, -i intval\n"
"      the amount of data in bytes 'playhrt' tries to read from the\n"
"      network during each loop (if needed). If not given or small\n"
"      'playhrt' uses the smallest amount it needs. You may try some\n"
"      larger value such that it is not necessary to read data during\n"
"      every loop.\n"
"\n"
"  --hw-buffer=intval, -c intval\n"
"      the buffer size (number of frames) used on the sound device.\n"
"      It may be worth to experiment a bit with this,\n"
"      in particular to try some smaller values. When 'playhrt' is\n"
"      called with --verbose it should report on the range allowed by\n"
"      the device. Default is 16384 (but there are devices where this\n"
"      is not valid).\n"
" \n"
"  --period-size=intval -P intval\n"
"      the period size is the chunk size (number of frames) read by the\n"
"      sound device. The default is chosen by the hardware driver.\n"
"      Use only for final tuning (or not at all), this option can cause\n"
"      strange behaviour.\n"
"\n"
"  --extra-bytes-per-second=floatval, -e floatval\n"
"      usually the number of bytes per second that must be written\n"
"      to the sound device is computed as the sample rate times the\n"
"      number of bytes per frame (i.e., one sample per channel).\n"
"      But the clocks of the computer and the DAC/Soundcard may\n"
"      differ a little bit. This parameter allows to specify a \n"
"      correction of this number of bytes per second (negative means\n"
"      to write a little bit slower, and positive to write a bit\n"
"      faster to the sound device). See ADJUSTING SPEED below for\n"
"      hints. The default is 0.\n"
"\n"
"  --no-delay-stats, -j\n"
"      disables statistics about delayed loops, see DELAYED LOOPS below.\n"
"      Only use this after finishing fine tuning of your parameters.\n"
"\n"
"  --no-buf-stats, -y\n"
"      in --mmap mode tries to self adjust and suggest better values\n"
"      of the --extra-bytes-per-second parameter by checking the average\n"
"      size of space available in the hardware buffer. The --no-buf-stats\n"
"      option disabled this check and adjustment. So, use this option\n"
"      only after finding the correct --extra-bytes-per-second parameter.\n"
"\n"
"  --in-net-buffer-size=intval, -N intval\n"
"      when reading from the network this allows to set the buffer\n"
"      size for the incoming data. This is for finetuning only, normally\n"
"      the operating system chooses sizes to guarantee constant data\n"
"      flow. The actual fill of the buffer during playback can be checked\n"
"      with 'netstat -tpn', it can be up to twice as big as the given\n"
"      intval.\n"
"\n"
"  --extra-frames-out=intval, -o intval\n"
"      when in one loop not all data were written the program has to\n"
"      write some additional frames the next time. This specifies the\n"
"      maximal extra amount before an underrun of data is assumed.\n"
"      Default is 24.\n"
"  \n"
"  --sleep=intval, -D intval\n"
"      causes playhrt to sleep for intval microseconds (1/1000000 sec)\n"
"      after opening the sound device and before starting playback.\n"
"      This may sometimes be useful to give other programs time to \n"
"      fill the input buffer of playhrt. Default is no sleep.\n"
"\n"
"  --max-bad-reads=intval, -m intval\n"
"      playhrt counts how often the read of a block of input data returns\n"
"      fewer data than requested. If this count exceeds the number given\n"
"      with this option then playhrt will stop. The default is 4 (because\n"
"      it is normal that the first block and one or two blocks at the end\n"
"      return fewer data).\n"
"\n"
"  --verbose, -v\n"
"      print some information during startup and operation.\n"
"      This option can be given twice for more output about timing\n"
"      and availability of the audio buffer (in --mmap mode).\n"
"\n"
"  --version, -V\n"
"      print information about the version of the program and abort.\n"
"\n"
"  --help, -h\n"
"      print this help page and abort.\n"
"\n"
"  EXAMPLES\n"
"\n"
"  We read from myserver on port 5123 stereo data in 32-bit integer\n"
"  format with a sample rate of 192000. We want to run 1000 loops per \n"
"  second (this is in particular a good choice for USB devices), our sound\n"
"  device is 'hw:0,0' and we want to write non-blocking to the device:\n"
"\n"
"  playhrt --mmap --host=myserver --port=5123 \\\n"
"      --loops-per-second=1000 \\\n"
"      --device=hw:0,0 --sample-rate=192000 --sample-format=S32_LE \\\n"
"      --non-blocking --verbose \n"
"\n"
"  To play a local CD quality flac file 'music.flac' you need another \n"
"  program to unpack the raw audio data. In this example we use 'sox':\n"
"\n"
"  sox musik.flac -t raw - | playhrt --mmap --stdin \\\n"
"          --loops-per-second=1000 --device=hw:0,0 --sample-rate=44100 \\\n"
"          --sample-format=S16_LE --non-blocking --verbose \n"
"\n"
"  Without the --mmap option playhrt can buffer the input data, and the\n"
"  size of the buffer can be chosen via the --buffer-size option.\n"
"\n"
"  ADJUSTING SPEED\n"
"\n"
"  Using the --mmap mode and a double --verbose --verbose option\n"
"  playhrt will show every few seconds how much space is available\n"
"  in the hardware buffer. This should stay roughly constant during\n"
"  playback. If it changes during playback or if a buffer underrun or\n"
"  overrun occurs then playhrt prints some suggestion for the value\n"
"  that should be given to the --extra-bytes-per-second option.\n"
"\n"
"  If you get an underrun or overrun without the --mmap option, you\n"
"  should enlarge or reduce  the --extra-bytes-per-second parameter \n"
"  by about:\n"
"      (value of --buffer-size) / (2 x number of seconds until underrun)\n"
"\n"
"  DELAYED LOOPS\n"
"\n"
"  It may happen that in some read-preparation-sleep loops the sleep starts\n"
"  after the intended wakeup time (too long computations, system interrupts\n"
"  or other reasons). playhrt counts such loops as \"delayed loops\" and\n"
"  reports them in --verbose mode. These can be ignored if their number is\n"
"  a small proportion of all loops, say, up to a few percent. If delayed\n"
"  lopps occur their number will become smaller with lower verbosity level\n"
"  and the detection of such loops can be disabled (and maybe further \n"
"  reduced with the --no-delay-stats option.\n"
"\n"
);
}


int main(int argc, char *argv[])
{
    int sfd, s, moreinput, err, verbose, nrchannels, startcount, sumavg,
        innetbufsize, dobufstats, countdelay, maxbad;
    long blen, hlen, ilen, olen, extra, loopspersec, nrdelays, sleep,
         nsec, count, wnext, badloops, badreads, readmissing, avgav, checkav;
    long long icount, ocount, badframes;
    void *buf, *iptr, *optr, *max;
    struct timespec mtime;
    struct timespec mtimecheck;
    double looperr, off, extraerr, extrabps, morebps;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_format_t format;
    char *host, *port, *pcm_name;
    int optc, nonblock, rate, bytespersample, bytesperframe;
    snd_pcm_uframes_t hwbufsize, periodsize, offset, frames;
    snd_pcm_access_t access;
    snd_pcm_sframes_t avail;
    const snd_pcm_channel_area_t *areas;
    double checktime;
    long corr;

    /* read command line options */
    static struct option longoptions[] = {
        {"host", required_argument, 0,  'r' },
        {"port", required_argument,       0,  'p' },
        {"stdin", no_argument,       0,  'S' },
        {"buffer-size", required_argument,       0,  'b' },
        {"input-size",  required_argument, 0, 'i'},
        {"loops-per-second", required_argument, 0,  'n' },
        {"sample-rate", required_argument, 0,  's' },
        {"sample-format", required_argument, 0, 'f' },
        {"number-channels", required_argument, 0, 'k' },
        {"mmap", no_argument, 0, 'M' },
        {"hw-buffer", required_argument, 0, 'c' },
        {"period-size", required_argument, 0, 'P' },
        {"device", required_argument, 0, 'd' },
        {"extra-bytes-per-second", required_argument, 0, 'e' },
        {"sleep", required_argument, 0, 'D' },
        {"max-bad-reads", required_argument, 0, 'm' },
        {"in-net-buffer-size", required_argument, 0, 'K' },
        {"extra-frames-out", required_argument, 0, 'o' },
        {"non-blocking-write", no_argument, 0, 'N' },
        {"overwrite", required_argument, 0, 'O' },
        {"verbose", no_argument, 0, 'v' },
        {"no-buf-stats", no_argument, 0, 'y' },
        {"no-delay-stats", no_argument, 0, 'j' },
        {"version", no_argument, 0, 'V' },
        {"help", no_argument, 0, 'h' },
        {0,         0,                 0,  0 }
    };

    if (argc == 1) {
       usage();
       exit(0);
    }
    /* defaults */
    host = NULL;
    port = NULL;
    blen = 65536;
    ilen = 0;
    loopspersec = 1000;
    rate = 44100;
    format = SND_PCM_FORMAT_S16_LE;
    bytespersample = 2;
    hwbufsize = 16384;
    periodsize = 0;
    /* nr of frames that wnext can be larger than olen */
    extra = 24;
    pcm_name = NULL;
    sfd = -1;
    nrchannels = 2;
    access = SND_PCM_ACCESS_RW_INTERLEAVED;
    extrabps = 0;
    sleep = 0;
    maxbad = 4;
    nonblock = 0;
    innetbufsize = 0;
    corr = 0;
    verbose = 0;
    dobufstats = 1;
    countdelay = 1;
    while ((optc = getopt_long(argc, argv, "r:p:Sb:i:n:s:f:k:Mc:P:d:e:o:NvVh",
            longoptions, &optind)) != -1) {
        switch (optc) {
        case 'r':
          host = optarg;
          break;
        case 'p':
          port = optarg;
          break;
        case 'S':
          sfd = 0;
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
        case 's':
          rate = atoi(optarg);
          break;
        case 'f':
          if (strcmp(optarg, "S16_LE")==0) {
             format = SND_PCM_FORMAT_S16_LE;
             bytespersample = 2;
          } else if (strcmp(optarg, "S24_LE")==0) {
             format = SND_PCM_FORMAT_S24_LE;
             bytespersample = 4;
          } else if (strcmp(optarg, "S24_3LE")==0) {
             format = SND_PCM_FORMAT_S24_3LE;
             bytespersample = 3;
          } else if (strcmp(optarg, "S32_LE")==0) {
             format = SND_PCM_FORMAT_S32_LE;
             bytespersample = 4;
          } else {
             fprintf(stderr, "playhrt: Sample format %s not recognized.\n", optarg);
             exit(1);
          }
          break;
        case 'k':
          nrchannels = atoi(optarg);
          break;
        case 'M':
          access = SND_PCM_ACCESS_MMAP_INTERLEAVED;
          break;
        case 'c':
          hwbufsize = atoi(optarg);
          break;
        case 'P':
          periodsize = atoi(optarg);
          break;
        case 'd':
          pcm_name = optarg;
          break;
        case 'e':
          extrabps = atof(optarg);
          break;
        case 'D':
          sleep = atoi(optarg);
          break;
        case 'm':
          maxbad = atoi(optarg);
          break;
        case 'K':
          innetbufsize = atoi(optarg);
          if (innetbufsize != 0 && innetbufsize < 128)
              innetbufsize = 128;
          break;
        case 'o':
          extra = atoi(optarg);
          break;
        case 'N':
          nonblock = 1;
          break;
        case 'O':
          break;
        case 'v':
          verbose += 1;
          break;
        case 'y':
          dobufstats = 0;
          break;
        case 'j':
          countdelay = 0;
          break;
        case 'V':
          fprintf(stderr,
                  "playhrt (version %s of frankl's stereo utilities",
                  VERSION);
#ifdef ALSANC
          fprintf(stderr, ", with alsa-lib patch");
#endif
          fprintf(stderr, ")\n");
          exit(0);
        default:
          usage();
          exit(2);
        }
    }
    bytesperframe = bytespersample*nrchannels;
    /* check some arguments and set some parameters */
    if ((host == NULL || port == NULL) && sfd < 0) {
       fprintf(stderr, "playhrt: Must specify --host and --port or --stdin.\n");
       exit(3);
    }
    /* compute nanoseconds per loop (wrt local clock) */
    extraerr = 1.0*bytesperframe*rate;
    extraerr = extraerr/(extraerr+extrabps);
    nsec = (int) (1000000000*extraerr/loopspersec);
    if (verbose) {
        fprintf(stderr, "playhrt: Step size is %ld nsec.\n", nsec);
    }
    /* olen in frames written per loop */
    olen = rate/loopspersec;
    if (olen <= 0)
        olen = 1;
    if (ilen < bytesperframe*(olen)) {
        if (olen*loopspersec == rate)
            ilen = bytesperframe * olen;
        else
            ilen = bytesperframe * (olen+1);
        if (verbose)
            fprintf(stderr, "playhrt: Setting input chunk size to %ld bytes.\n", ilen);
    }
    /* need big enough input buffer */
    if (blen < 3*ilen) {
        blen = 3*ilen;
    }
    hlen = blen/2;
    if (olen*loopspersec == rate)
        looperr = 0.0;
    else
        looperr = (1.0*rate)/loopspersec - 1.0*olen;
    moreinput = 1;
    icount = 0;
    ocount = 0;
    /* for mmap try to set hwbuffer to multiple of output per loop */
    if (access == SND_PCM_ACCESS_MMAP_INTERLEAVED) {
        hwbufsize = hwbufsize - (hwbufsize % olen);
    }

    /* need blen plus some overlap for (circular) input buffer */
    if (! (buf = malloc(blen+ilen+(olen+extra)*bytesperframe)) ) {
        fprintf(stderr, "playhrt: Cannot allocate buffer of length %ld.\n",
                blen+ilen+(olen+extra)*bytesperframe);
        exit(2);
    }
    if (verbose) {
        fprintf(stderr, "playhrt: Input buffer size is %ld bytes.\n",
                blen+ilen+(olen+extra)*bytesperframe);
    }
    /* we put some overlap before the reference pointer */
    buf = buf + (olen+extra)*bytesperframe;
    max = buf + blen;
    /* the pointers for next input and next output */
    iptr = buf;
    optr = buf;

    /* setup network connection */
    if (host != NULL && port != NULL) {
        sfd = fd_net(host, port);
        if (innetbufsize != 0) {
            if (setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, (void*)&innetbufsize, sizeof(int)) < 0) {
                fprintf(stderr, "playhrt: Cannot set buffer size for network socket to %d.\n",
                        innetbufsize);
                exit(23);
            }
        }
    }

    /* setup sound device */
    snd_pcm_hw_params_malloc(&hwparams);
    if (snd_pcm_open(&pcm_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf(stderr, "playhrt: Error opening PCM device %s\n", pcm_name);
        exit(5);
    }
    if (nonblock) {
        if (snd_pcm_nonblock(pcm_handle, 1) < 0) {
            fprintf(stderr, "playhrt: Cannot set non-block mode.\n");
            exit(6);
        } else if (verbose) {
            fprintf(stderr, "playhrt: Using card in non-block mode.\n");
        }
    }
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "playhrt: Cannot configure this PCM device.\n");
        exit(7);
    }
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, access) < 0) {
        fprintf(stderr, "playhrt: Error setting access.\n");
        exit(8);
    }
    if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, format) < 0) {
        fprintf(stderr, "playhrt: Error setting format.\n");
        exit(9);
    }
    if (snd_pcm_hw_params_set_rate(pcm_handle, hwparams, rate, 0) < 0) {
        fprintf(stderr, "playhrt: Error setting rate.\n");
        exit(10);
    }
    if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, nrchannels) < 0) {
        fprintf(stderr, "playhrt: Error setting channels to %d.\n", nrchannels);
        exit(11);
    }
    if (periodsize != 0) {
      if (snd_pcm_hw_params_set_period_size(
                                pcm_handle, hwparams, periodsize, 0) < 0) {
          fprintf(stderr, "playhrt: Error setting period size to %ld.\n", periodsize);
          exit(11);
      }
      if (verbose) {
          fprintf(stderr, "playhrt: Setting period size explicitly to %ld frames.\n",
                          periodsize);
      }
    }
    if (verbose) {
        snd_pcm_uframes_t min=1, max=100000000;
        snd_pcm_hw_params_set_buffer_size_minmax(pcm_handle, hwparams,
                                                                &min, &max);
        fprintf(stderr,
                "playhrt: Min and max buffer size of device %ld .. %ld - ", min, max);
    }
    if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams,
                                                      hwbufsize) < 0) {
        fprintf(stderr, "\nplayhrt: Error setting buffersize to %ld.\n", hwbufsize);
        exit(12);
    }
    snd_pcm_hw_params_get_buffer_size(hwparams, &hwbufsize);
    if (verbose) {
        fprintf(stderr, " using %ld.\n", hwbufsize);
    }
    if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "playhrt: Error setting HW params.\n");
        exit(13);
    }
    snd_pcm_hw_params_free(hwparams);
    if (snd_pcm_sw_params_malloc (&swparams) < 0) {
        fprintf(stderr, "playhrt: Cannot allocate SW params.\n");
        exit(14);
    }
    if (snd_pcm_sw_params_current(pcm_handle, swparams) < 0) {
        fprintf(stderr, "playhrt: Cannot get current SW params.\n");
        exit(15);
    }
    if (snd_pcm_sw_params_set_start_threshold(pcm_handle,
                                          swparams, hwbufsize/2) < 0) {
        fprintf(stderr, "playhrt: Cannot set start threshold.\n");
        exit(16);
    }
    if (snd_pcm_sw_params(pcm_handle, swparams) < 0) {
        fprintf(stderr, "playhrt: Cannot apply SW params.\n");
        exit(17);
    }
    snd_pcm_sw_params_free (swparams);

    /* main loop */
    badloops = 0;
    badframes = 0;
    badreads = 0;
    readmissing = 0;
    nrdelays = 0;

    /* short delay to allow input to fill buffer */
    if (sleep > 0) {
      mtime.tv_sec = sleep/1000000;
      mtime.tv_nsec = 1000*(sleep - mtime.tv_sec*1000000);
      nanosleep(&mtime, NULL);
    }

    if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
      /* fill half buffer */
      for (; iptr < buf + 2*hlen - ilen; ) {
          memclean(iptr, ilen);
          s = read(sfd, iptr, ilen);
          if (s < 0) {
              fprintf(stderr, "playhrt: Read error.\n");
              exit(18);
          }
          icount += s;
          if (s == 0) {
              moreinput = 0;
              break;
          }
          iptr += s;
      }
      if (iptr - optr < olen*bytesperframe)
          wnext = (iptr-optr)/bytesperframe;
      else
          wnext = olen;

      if (clock_gettime(CLOCK_MONOTONIC, &mtime) < 0) {
          fprintf(stderr, "playhrt: Cannot get monotonic clock.\n");
          exit(19);
      }
      if (verbose)
         fprintf(stderr, "playhrt: Start time (%ld sec %ld nsec).\n",
                         mtime.tv_sec, mtime.tv_nsec);
      for (count=1, off=looperr; 1; count++, off+=looperr) {
          /* compute time for next wakeup */
          mtime.tv_nsec += nsec;
          if (mtime.tv_nsec > 999999999) {
            mtime.tv_nsec -= 1000000000;
            mtime.tv_sec++;
          }
          refreshmem(optr, wnext*bytesperframe);
          refreshmem(optr, wnext*bytesperframe);
          clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mtime, NULL);
          /* write a chunk, this comes first immediately after waking up */
#ifdef ALSANC
          /* here we use snd_pcm_writei_nc (if available in patched ALSA
             library. This avoids some error checks and high cpu usage with
             small hardware buffer sizes */
          s = snd_pcm_writei_nc(pcm_handle, optr, wnext);
#else
          /* otherwise we use the standard snd_pcm_writei  */
          s = snd_pcm_writei(pcm_handle, optr, wnext);
#endif
          while (s < 0) {
              s = snd_pcm_recover(pcm_handle, s, 0);
              if (s < 0) {
                  snd_pcm_prepare(pcm_handle);
                  fprintf(stderr, "playhrt: <<<<< Cannot write, resetted >>>>\n");
              }
              clock_gettime(CLOCK_MONOTONIC, &mtime);
              if (verbose)
                 fprintf(stderr, "playhrt: Bad write at (%ld sec %ld nsec).\n",
                         mtime.tv_sec, mtime.tv_nsec);
#ifdef ALSANC
              s = snd_pcm_writei_nc(pcm_handle, optr, wnext);
#else
              s = snd_pcm_writei(pcm_handle, optr, wnext);
#endif
          }
          /* we count output and bad loops */
          if (s < wnext) {
              badloops++;
              badframes += (wnext - s);
          }
          ocount += s*bytesperframe;
          optr += s*bytesperframe;
          wnext = olen + wnext - s;
          if (off >= 1.0) {
             off -= 1.0;
             wnext++;
          }
          if (wnext >= olen+extra) {
             if (verbose)
                fprintf(stderr, "playhrt: Underrun by %ld bytes at (%ld sec %ld nsec).\n",
                        wnext - olen - extra, mtime.tv_sec, mtime.tv_nsec);
             wnext = olen+extra-1;
          }
          s = (iptr >= optr ? iptr - optr : iptr+blen-optr);
          if (s <= wnext*bytesperframe) {
              wnext = s/bytesperframe;
          }
          if (optr+wnext*bytesperframe >= max) {
              optr -= blen;
          }
          /* read if buffer not half filled */
          if (moreinput && (iptr > optr ? iptr-optr : iptr+blen-optr) < hlen) {
              memclean(iptr, ilen);
              s = read(sfd, iptr, ilen);
              if (s < 0) {
                  fprintf(stderr, "playhrt: Read error.\n");
                  exit(20);
              } else if (s < ilen) {
                  badreads++;
                  readmissing += (ilen-s);
              }
              icount += s;
              iptr += s;
              /* copy input to beginning if we reach end of buffer */
              if (iptr >= max) {
                  memcpy(buf-(olen+extra)*bytesperframe,
                                           max-(olen+extra)*bytesperframe,
                                           iptr-max+(olen+extra)*bytesperframe);
                  iptr -= blen;
              }
              if (s == 0) { /* input complete */
                  moreinput = 0;
              }
          }
          if (wnext == 0)
              break;    /* done */
      }
    } else if (access == SND_PCM_ACCESS_MMAP_INTERLEAVED) {
      /* mmap access */
      /* why does start threshold not work ??? */
     if (verbose)
         fprintf(stderr, "playhrt: Using mmap access.\n");
     startcount = hwbufsize/(2*olen);
     if (clock_gettime(CLOCK_MONOTONIC, &mtime) < 0) {
          fprintf(stderr, "playhrt: Cannot get monotonic clock.\n");
          exit(19);
      }
      if (verbose)
         fprintf(stderr, "playhrt: Start time (%ld sec %ld nsec).\n",
                         mtime.tv_sec, mtime.tv_nsec);
      sumavg= 0;
      checktime = 0;
      for (count=1, off=looperr; 1; count++, off+=looperr) {
          /* start playing when half of hwbuffer is filled */
          if (count == startcount)  snd_pcm_start(pcm_handle);

          frames = olen;
          if (off > 1.0) {
              frames++;
              off -= 1.0;
          }
          avail = snd_pcm_avail_update(pcm_handle);
          err = snd_pcm_mmap_begin(pcm_handle, &areas, &offset, &frames);
          if (err < 0) {
              fprintf(stderr, "playhrt: Don't get mmap address.\n");
              exit(21);
          }

          /* do some statistics to check average hwbuffer space available
             to check and improve --extra-bytes-per-second parameter */
          if (dobufstats && count > startcount && count % 4096 == 0) {
              sumavg = 16;
              avgav = 0;
          }
          if (sumavg) {
              avgav += avail;
              if (sumavg == 1) {
                  if (verbose > 1)
                      fprintf(stderr, "playhrt: Average available buffer: %ld (%ld sec %ld nsec).\n", avgav/16, mtime.tv_sec, mtime.tv_nsec);
                  if (checktime == 0.0 && count > startcount+30000) {
                       checktime = 1.0*mtime.tv_sec + mtime.tv_nsec/1000000000.0;
                       checkav = avgav/16;
                       corr = 1;
                  }
                  if (corr && avgav/16 > checkav + hwbufsize*3/10) {
                       extrabps += (double)((avgav/16-checkav)*bytesperframe)/(mtime.tv_sec*1.0+mtime.tv_nsec/1000000000.0-checktime);
                       extraerr = 1.0*bytesperframe*rate;
                       extraerr = extraerr/(extraerr+extrabps);
                       nsec = (int) (1000000000*extraerr/loopspersec);
                       corr = 0;
                       fprintf(stderr, "playhrt: Avoiding buffer underrun! Please use option \n"
                               "      --extra-bytes-per-second=%d\n"
                               "on next call.\n", (int)extrabps);
                  }
                  if (corr && avgav/16 < checkav - hwbufsize*3/10) {
                       extrabps += (double)((avgav/16-checkav)*bytesperframe)/(mtime.tv_sec*1.0+mtime.tv_nsec/1000000000.0-checktime);
                       extraerr = 1.0*bytesperframe*rate;
                       extraerr = extraerr/(extraerr+extrabps);
                       nsec = (int) (1000000000*extraerr/loopspersec);
                       corr = 0;
                       fprintf(stderr, "playhrt: Avoiding buffer overrun! Please use option \n"
                               "      --extra-bytes-per-second=%d\n"
                               "on next call.\n", (int)extrabps);
                  }
              }
              sumavg--;
          }

          ilen = frames * bytesperframe;
          iptr = areas[0].addr + offset * bytesperframe;
          /*memclean(iptr, ilen);  commented out to save some CPU-time */
          /* in --mmap mode we read directly into mmaped space without internal buffer */
          s = read(sfd, iptr, ilen);

          /* compute time for next wakeup */
          mtime.tv_nsec += nsec;
          if (mtime.tv_nsec > 999999999) {
            mtime.tv_nsec -= 1000000000;
            mtime.tv_sec++;
          }


          /* we refresh the new data before and directly after the  sleep before commiting */
          refreshmem(iptr, s);

          /* debug:  check that we really sleep to some time in the future */
          if (countdelay) {
            clock_gettime(CLOCK_MONOTONIC, &mtimecheck);
            if (mtimecheck.tv_sec > mtime.tv_sec || (mtimecheck.tv_sec == mtime.tv_sec && mtimecheck.tv_nsec > mtime.tv_nsec))
                nrdelays += 1;
          }
          if (verbose > 1 && nrdelays > 0 && count % 4096 == 0) {
              fprintf(stderr, "playhrt: Number of delayed loops: %ld (%ld sec %ld nsec).\n", nrdelays, mtime.tv_sec, mtime.tv_nsec);
          }

          clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mtime, NULL);
	  refreshmem(iptr, s);
          snd_pcm_mmap_commit(pcm_handle, offset, frames);
          if (s < 0) {
              fprintf(stderr, "playhrt: Read error.\n");
              exit(22);
          } else if (s < ilen) {
              badreads++;
              readmissing += (ilen-s);
              if (verbose)
                  fprintf(stderr, "playhrt: Bad read, %ld bytes missing at %ld.%ld.\n", (ilen-s), mtime.tv_sec, mtime.tv_nsec);
              if (badreads >= maxbad) {
                  fprintf(stderr, "playhrt: Had %d bad reads . . . exiting.\n", maxbad);
                  break;
              }
          }
          icount += s;
          ocount += s;
          if (s == 0) /* done */
              break;
      }
    }
    /* cleanup network connection and sound device */
    close(sfd);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    if (verbose) {
        if (corr) {
            morebps = (double)((avgav/16-checkav)*bytesperframe)/(mtime.tv_sec*1.0+mtime.tv_nsec/1000000000.0-checktime);
            if (morebps >= 1.0 || morebps <= -1.0)
                fprintf(stderr, "playhrt: Suggesting option \n"
                             "      --extra-bytes-per-second=%d\n"
                             "on future calls.\n", (int)(extrabps+morebps));
        }
        fprintf(stderr, "playhrt: Loops: %ld (%ld delayed), total bytes: %lld in %lld out. \n"
                        "playhrt: Bad loops/frames written: %ld/%lld,  bad reads/bytes: %ld/%ld.\n",
                    count, nrdelays, icount, ocount, badloops, badframes, badreads, readmissing);
    }
    return 0;
}


