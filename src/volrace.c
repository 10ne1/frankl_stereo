/*
volrace.c                Copyright frankl 2013-2014

This file is part of frankl's stereo utilities.
See the file License.txt of the distribution and
http://www.gnu.org/licenses/gpl.txt for license details.
*/

#include "version.h"
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define LEN 100000
#define MAXDELAY 500
#define FALSE   0
#define TRUE    1

/* help page */
/* vim hint to remove resp. add quotes:
      s/^"\(.*\)\\n"$/\1/
      s/.*$/"\0\\n"/
*/
void usage( ) {
  fprintf(stderr,
          "volrace (version %s of frankl's stereo utilities)\nUSAGE:\n",
          VERSION);
  fprintf(stderr,
"\n"
"  volrace [options] \n"
"\n"
"  This command works as a filter for stereo audio streams\n"
"  in raw double or float format. It can change the volume\n"
"  and it implements a simple form of the RACE algorithm (see\n"
"  http://www.ambiophonics.org/Tutorials/RGRM-RACE_rev.html).\n"
"\n"
"  Here 'filter' means that input comes from stdin and output is\n"
"  written to stdout. Use pipes and 'sox' to deal with other stream\n"
"  formats. Internal calculations are done with 64-bit floating point \n"
"  numbers.\n"
"\n"
"  The filter parameters can be given as options or be read from a file.\n"
"  In the latter case the file is reread if it was changed.\n"
"\n"
"  RACE\n"
"\n"
"  This is an IIR-filter, which adds the inverted and attenuated \n"
"  signal of the left channel with a delay to the right channel and\n"
"  vice versa. (In a former version we also provided an optional\n"
"  simple low-pass and high-pass filter for the signal copied to the \n"
"  other channel. But results where not convincing, therefore this is\n"
"  not supported by this version.)\n"
"\n"
"  The origin of this algorithm is ambiophonics which is for speakers\n"
"  which are positioned closely together (opening angle < 20 degrees\n"
"  from the listening position). In this context typical suggested\n"
"  values for the attenuation would be around 3dB (factor 0.71) and\n"
"  a delay of about 60-120 microseconds (3-6 samples for sample rate\n"
"  44100, or 11-21 samples for sample rate 192000).\n"
"\n"
"  The author uses RACE in a standard stereo setup (speakers in 70 \n"
"  degree angle) with a 57 sample delay for sample rate 192000 and \n"
"  an attenuation of 12dB-40dB (depending on the recording). \n"
"\n"
"  On a notebook with builtin speakers a delay of 4 samples at sample\n"
"  rate 44100 and an attenuation of 3dB give an interesting effect.\n"
"\n"
"  OPTIONS\n"
"  \n"
"  --volume=floatval, -v floatval\n"
"      the volume as floating point factor (e.g., '0.5' for an attenuation\n"
"      of 6dB). Here floatval can be negative in which case the audio stream\n"
"      will be inverted. Default is 0.01, and the absolute value of floatval\n"
"      must not be larger than max-volume, see below.\n"
"\n"
"  --race-delay=val, -d val\n"
"      the delay for RACE as (integer) number of samples (per channel).\n"
"      Default is 12.\n"
"\n"
"  --race-volume=floatval, -a floatval\n"
"      the volume of the RACE signal copied to the other channel.\n"
"      Default is 0.0 (that is, no RACE effect).\n"
"\n"
"  --param-file=fname, -f fname\n"
"      the name of a file which can be given instead of the previous three\n"
"      options. That file must contain the values for --volume, \n"
"      --race-delay and --race-volume, separated by whitespace.\n"
"      This file is reread by the program when it was changed, and the\n"
"      parameters are faded to the new values. This way this program can\n"
"      be used as volume (and RACE parameter) control during audio playback.\n"
"      The file may only contain the value of --volume, in this case RACE\n"
"      will be disabled.\n"
"\n"
"  --buffer-length=intval, -b intval\n"
"      the length of the chunks read and written. Default is 8192\n"
"      frames which should usually be fine.\n"
"\n"
"  --max-volume=floatval, -m floatval\n"
"      just for security, the program will refuse to set the absolute\n"
"      value of --volume larger than this. Default is 1.0.\n"
"\n"
"  --fading-length=intval, -l intval\n"
"      number of frames used for fading to new parameters (when the\n"
"      file given in --param-file was changed). Default is 44100 which \n"
"      usually should be fine.\n"
"\n"
"  --float-input, -I\n"
"      by default the input of the program is assumed to consist of\n"
"      64-bit floating point samples. Use this switch if your input is\n"
"      in 32-bit floating point format.\n"
"\n"
"  --float-output, -O\n"
"      by default the output of the program consists of 64-bit floating \n"
"      point samples. Use this switch if you want 32-bit floating point \n"
"      samples in the output.\n"
"      \n"
"   --help, -h\n"
"      show this help.\n"
"\n"
"   --verbose, -p\n"
"      shows some information during startup and operation.\n"
"\n"
"   --version, -V\n"
"      show the version of this program and exit.\n"
"\n"
"   EXAMPLES\n"
"\n"
"   For volume and RACE control during audio playback:\n"
"      volrace --param-file=/tmp/vrparams\n"
"   where /tmp/vrparams contains something like\n"
"      -0.3 57 0.22\n"
"   (invert, reduce volume by factor 0.3, use RACE with delay 57 frames \n"
"    and volume 0.22 which is about -13dB).\n"
"   Or give these values directly (cannot be changed while running):\n"
"      volrace --volume=-0.3 --race-delay=57 --race-volume=0.22\n"
"   Apply RACE to a CD quality flac-file x.flac and store it as wav-file:\n"
"      sox x.flac -t raw -e float -b 64 - | \\\n"
"        volrace --volume=-0.7 --race-delay=13 --race-volume=0.22 | \\\n"
"        sox -t raw -r 44100 -c 2 -e float -b 64 - -t wav -e signed -b 16 \\\n"
"            x_race.wav dither -f high-shibata \n"
"\n"
);
}

/* utility to get modification time of a file in nsec precision */
double mtimens (char* fnam) {
  struct stat sb;
  if (stat(fnam, &sb) == -1)
     return 0.0;
  else
    return (double)sb.st_mtim.tv_sec + (double) sb.st_mtim.tv_nsec*0.000000001;
}

/* read parameters for vol, delay, att from file
   use init==1 during initialization, program will terminate in case of
   problem; later use init==0, if a problem occurs, the parameters are
   just left as they are                                                */
int getparams(char* fnam, double* vp, int* delay, double* att, int init) {
  FILE* params;
  int ok;
  params = fopen(fnam, "r");
  if (!params) {
     if (init) {
       fprintf(stderr, "volrace: Cannot open %s\n", fnam);
       fflush(stderr);
       exit(2);
     } else
       return 0;
  }
  ok = fscanf(params, "%lf %d %lf", vp, delay, att);
  if (ok == EOF || ok == 0) {
     fclose(params);
     if (init) {
       fprintf(stderr, "volrace: Cannot read parameters from  %s\n", fnam);
       fflush(stderr);
       exit(3);
     }  else
       return 0;
  }
  if (ok < 3) {
     /* allow only volume in file, disable RACE */
     *delay = 13;
     *att = 0.0;
  }
  fclose(params);
  return 1;
}

/* correct too bad values */
void sanitizeparams(double maxvol, double* vp, int* delay, double* att, int bl){
  if (*vp < -maxvol || *vp > maxvol) {
     fprintf(stderr, "volrace: Invalid vol, using %.4f\n", 0.01*maxvol); fflush(stderr);
     *vp = 0.01*maxvol;
  }
  if (*delay < 1 || *delay > MAXDELAY) {
     fprintf(stderr, "volrace: Invalid delay, using 12\n"); fflush(stderr);
     *delay = 12;
  }
  if (*att < -0.95 || *att > 0.95) {
     fprintf(stderr, "volrace: Invalid att, using 0.0 (disabled)\n"); fflush(stderr);
     *att = 0.25;
  }
  if (bl < 2 * (*delay)) {
     fprintf(stderr, "volrace: buffer to short for delay, using 12\n"); fflush(stderr);
     *delay = 12;
  }
}

int main(int argc, char *argv[])
{
  double att, vol, maxvol, ptime=0.0, natt, nvol, ntime, vdiff, adiff;
  double buf[2*LEN+2*MAXDELAY], inp[2*LEN], out[2*LEN];
  float inpfloat[2*LEN], outfloat[2*LEN];
  int optc, optind, blen, delay, ndelay, i, check, mlen, change, count,
      fadinglength, verbose;
  char *fnam, floatin, floatout;

  if (argc == 1) {
      usage();
      exit(1);
  }
  /* read command line options */
  static struct option longoptions[] = {
      {"volume", required_argument, 0,  'v' },
      {"race-delay", required_argument,       0,  'd' },
      {"race-volume", required_argument, 0,  'a' },
      {"bufferlength", required_argument,       0,  'b' },
      {"param-file",  required_argument, 0, 'f'},
      {"max-volume", required_argument, 0,  'm' },
      {"fading-length", required_argument, 0,  'l' },
      {"float-input", no_argument, 0, 'I' },
      {"float-output", no_argument, 0, 'O' },
      {"verbose", no_argument, 0, 'p' },
      {"version", no_argument, 0, 'V' },
      {"help", no_argument, 0, 'h' },
      {0,         0,                 0,  0 }
  };
  /* defaults */
  vol = 0.01;
  delay = 12;
  att = 0.0;
  blen = 8192;
  fnam = NULL;
  maxvol = 1.0;
  fadinglength = 44100;
  verbose = 0;
  floatin = FALSE;
  floatout = FALSE;
  while ((optc = getopt_long(argc, argv, "v:d:a:b:f:m:l:IOVh",
          longoptions, &optind)) != -1) {
      switch (optc) {
      case 'v':
        vol = atof(optarg);
        break;
      case 'd':
        delay = atoi(optarg);
        break;
      case 'a':
        att = atof(optarg);
        break;
      case 'b':
        blen = atoi(optarg);
        if (blen < 1024 || blen > LEN) {
           fprintf(stderr, "volrace: Strange buffer length, using 8192 . . .\n");
           fflush(stderr);
           blen = 8192;
        }
        break;
      case 'f':
        fnam = strdup(optarg);
        getparams(fnam, &vol, &delay, &att, 1);
        break;
      case 'm':
        maxvol = atof(optarg);
        break;
      case 'l':
        fadinglength = atoi(optarg);
        if (fadinglength < 1 || fadinglength > 400000) {
           fprintf(stderr, "volrace: Strange fading length, using 44100 . . .\n");
           fflush(stderr);
           fadinglength = 44100;
        }
        break;
      case 'p':
        verbose = 1;
        break;
      case 'I':
        floatin = TRUE;
        break;
      case 'O':
        floatout = TRUE;
        break;
      case 'V':
        fprintf(stderr, "volrace (version %s of frankl's stereo utilities)\n",
                VERSION);
        exit(0);
      default:
        usage();
        exit(1);
      }
  }
  sanitizeparams(maxvol, &vol, &delay, &att, blen);

  /* remember modification time of parameter file */
  if (fnam)
    ptime = mtimens(fnam);

  if (verbose) {
     fprintf(stderr, "verbose:");
     if (floatin)
        fprintf(stderr, " float input,");
     if (floatout)
        fprintf(stderr, " float output,");
     if (fnam)
        fprintf(stderr, " parameters from file,");
     fprintf(stderr, "vol %.3f, race att %.3f delay %ld\n", (double)vol, (double)att, (long)delay);
  }

  /* if count >= 0 we are fading the parameters to a new value */
  vdiff = 0.0;
  adiff = 0.0;
  count = -1;
  /* prepend delay zero samples */
  for (i=0; i<2*delay; i++) buf[i] = 0.0;

  /* we read from stdin until eof and write to stdout */
  while (TRUE) {
    if (floatin) {
      mlen = fread((void*)inpfloat, 2*sizeof(float), blen, stdin);
      for (i=0; i<2*mlen; i++)
        inp[i] = (double)(inpfloat[i]);
    } else
      mlen = fread((void*)inp, 2*sizeof(double), blen, stdin);
    if (mlen == 0)  /* done */
      break;
    for (i=0; i<mlen; i++) {
      /* copy into buf with delayed race signal */
      buf[2*(i+delay)] = vol*inp[2*i] - att*buf[2*i+1];
      buf[2*(i+delay)+1] = vol*inp[2*i+1] - att*buf[2*i];
      if (floatout) {
        outfloat[2*i] = (float)(buf[2*(i+delay)]);
        outfloat[2*i+1] = (float)(buf[2*(i+delay)+1]);
      } else {
        out[2*i] = buf[2*(i+delay)];
        out[2*i+1] = buf[2*(i+delay)+1];
      }
      /* change parameters slightly while fading to new ones */
      if (count > 0) {
        vol += vdiff;
        att += adiff;
        count--;
      } else if (count == 0) {
        /* now set vol and att to precisely the new values and disable
           fading by setting count == -1 */
        vol = nvol;
        att = natt;
        count--;
      }
    }
    if (floatout)
      check = fwrite((void*)outfloat, 2*sizeof(float), mlen, stdout);
    else
      check = fwrite((void*)out, 2*sizeof(double), mlen, stdout);
    fflush(stdout);
    /* this should not happen, the whole block should be written */
    if (check < mlen) {
        fprintf(stderr, "volrace: error in write\n");
        fflush(stderr);
        exit(4);
    }

    /* if we are not fading to new parameters we check after each block
       if the modification time of the parameter file has changed; if yes
       we try to read new values; in case of a problem we stay with the old
       parameters */
    if (fnam != NULL && count < 0) {
        ntime = mtimens(fnam);
        if (ntime > ptime+0.00001) {
           change = getparams(fnam, &nvol, &ndelay, &natt, 0);
           if (change) {
             sanitizeparams(maxvol, &nvol, &ndelay, &natt, blen);
             /* we fade to new values within a number of frames */
             count = fadinglength;
             vdiff = (nvol-vol)/count;
             adiff = (natt-att)/count;
             ptime = ntime;
             delay = ndelay;
             if (verbose) {
                fprintf(stderr, "verbose: reread new parameters: (%f) ", ntime);
                fprintf(stderr, "vol %.3f, race att %.3f delay %ld\n", (double)nvol, (double)natt, (long)ndelay);
             }
           }
        }
    }

    /* move last delay samples to front */
    for (i=0; i<2*delay; i++) buf[i] = buf[2*mlen+i];
  }
  return(0);
}

