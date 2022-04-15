/* main.c
 * Command line interface.
 *
 * This fixed point version of shine is based on Gabriel Bouvigne's original
 * source, version 0.1.2
 * It was converted for use on Acorn computers running RISC OS and will require
 * the assembler multiply file to be replaced for other platforms.
 * 09/02/01 P.Everett 
 */

//#define RISCOS
 
//int errno;
 
#include "Types.h"
#include "Wave.h"
#include "Layer3.h"

config_t config;
static int raw;

/* RISC OS specifics */
#define AMPEG 0x1ad      /* Audio MPEG filetype */
#define WAVE  0xfb1      /* Wave filetype */
#define DATA  0xffd      /* Data filetype */
#ifdef RISCOS
extern void settype(char *name, int type);
extern int readtype(char *name);
#endif

/*
 * error:
 * ------
 */
void error(char *s)
{
#if !defined(SHINE_SILENT_MODE)
  printf("[ERROR] %s\n",s);
#endif
  exit(1);
}

/*
 * print_usage:
 * ------------
 */
static void print_usage()
{
#if !defined(SHINE_SILENT_MODE)
  printf("USAGE   :  Shine [options] <infile> <outfile>\n");
  printf("options : -h            this help message\n");
  printf("          -b <bitrate>  set the bitrate [32-320], default 128kbit\n");
  printf("          -c            set copyright flag, default off\n");
  printf("          -r            raw cd data file instead of wave\n");
  printf("\n"); 
#endif
}

/*
 * set_defaults:
 * -------------
 */
static void set_defaults()
{
  config.mpeg.type = 1;
  config.mpeg.layr = 2;
  config.mpeg.mode = 2;
  config.mpeg.bitr = 128;
  config.mpeg.psyc = 2;
  config.mpeg.emph = 0; 
  config.mpeg.crc  = 0;
  config.mpeg.ext  = 0;
  config.mpeg.mode_ext  = 0;
  config.mpeg.copyright = 0;
  config.mpeg.original  = 1;
}

/*
 * parse_command:
 * --------------
 */
static bool parse_command(int argc, char** argv)
{
  int i = 0;

  if(argc<3) return false;

  raw = false;
  
  while(argv[++i][0]=='-')
    switch(argv[i][1])
    {
      case 'b':
        config.mpeg.bitr = atoi(argv[++i]);
        break;
        
      case 'c':
        config.mpeg.copyright = 1;
        break;
        
      case 'r':
        raw = true;
        break;
        
      case 'h':
      default :
        return false;
    }

  if((argc-i)!=2) return false;
  config.infile  = argv[i++];
  config.outfile = argv[i];
  return true;
}

/*
 * find_samplerate_index:
 * ----------------------
 */
static int find_samplerate_index(long freq)
{
  static long mpeg1[3] = {44100, 48000, 32000};
  int i;

  for(i=0;i<3;i++)
    if(freq==mpeg1[i]) return i;

  error("Invalid samplerate");
}

/*
 * find_bitrate_index:
 * -------------------
 */
static int find_bitrate_index(int bitr)
{
  static long mpeg1[15] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
  int i;

  for(i=0;i<15;i++)
    if(bitr==mpeg1[i]) return i;

  error("Invalid bitrate");
}

/*
 * check_config:
 * -------------
 */
static void check_config()
{
  static char *mode_names[4]    = { "stereo", "j-stereo", "dual-ch", "mono" };
  static char *layer_names[3]   = { "I", "II", "III" };
  static char *version_names[2] = { "MPEG-II (LSF)", "MPEG-I" };
  static char *psy_names[3]     = { "", "MUSICAM", "Shine" };
  static char *demp_names[4]    = { "none", "50/15us", "", "CITT" };

  config.mpeg.samplerate_index = find_samplerate_index(config.wave.samplerate);
  config.mpeg.bitrate_index    = find_bitrate_index(config.mpeg.bitr);

#if !defined(SHINE_SILENT_MODE)
  printf("%s layer %s, %s  Psychoacoustic Model: %s\n",
           version_names[config.mpeg.type],
           layer_names[config.mpeg.layr], 
           mode_names[config.mpeg.mode],
           psy_names[config.mpeg.psyc]);
  printf("Bitrate=%d kbps  ",config.mpeg.bitr );
  printf("De-emphasis: %s   %s %s\n",
          demp_names[config.mpeg.emph], 
          ((config.mpeg.original)?"Original":""),
          ((config.mpeg.copyright)?"(C)":""));
#endif
}

/*
 * main:
 * -----
 */
int main(int argc, char **argv)
{
  time_t end_time;
  int filetype;

  time(&config.start_time);
#if !defined(SHINE_SILENT_MODE)
  printf("ARM Shine v1.00(SA) 24/03/01\n");
#endif
  set_defaults();

  if(!parse_command(argc,argv))
  {
    print_usage();
    exit(1);
  }

#ifdef RISCOS
  filetype = readtype(config.infile);
#else
  filetype = WAVE;
#endif
  
  if(raw)
    wave_open(DATA);
  else
  {
    switch(filetype)
    {
      case WAVE:
      case DATA:
        wave_open(filetype);
        break;
        
      default: wave_open(WAVE);
    }
  }

  check_config();

#if !defined(SHINE_SILENT_MODE)
  printf("Encoding \"%s\" to \"%s\"\n", config.infile, config.outfile);
#endif        
  L3_compress();
    
#ifdef RISCOS
  settype(config.outfile, AMPEG);
#endif
  
  wave_close();

  time(&end_time);
  end_time -= config.start_time;
#if !defined(SHINE_SILENT_MODE)
  printf(" Finished in %2ld:%2ld:%2ld\n",
            end_time/3600,(end_time/60)%60,end_time%60);
  exit(0);
#endif
} 

