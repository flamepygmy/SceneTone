/* wave.c
 *
 * MS Wave files store data 'little endian' sytle. These functions will only
 * work on 'little endian' machines. 09/02/01 P.Everett
 * note. both Acorn/RISC OS and PC/DOS are little endian.
 */
 
#include "Types.h"
#include "Wave.h"

/* RISC OS specifics */
#define WAVE  0xfb1      /* Wave filetype */
#define DATA  0xffd      /* Data filetype */

/*
 * wave_close:
 * -----------
 */
void wave_close(void)
{
  fclose(config.wave.file);
}

/*
 * wave_open:
 * ----------
 * Opens and verifies the header of the Input Wave file. The file pointer is
 * left pointing to the start of the samples.
 */
void wave_open(int filetype)
{

  static char *channel_mappings[] = {NULL,"mono","stereo"};
  static char *file_type[] = {"RIFF-WAVE PCM","RAW CD DATA PCM"};
  int i;

  /* Wave file headers can vary from this, but we're only intereseted in this format */
  struct wave_header
  {
    char riff[4];             /* "RIFF" */
    unsigned long size;       /* length of rest of file = size of rest of header(36) + data length */
    char wave[4];             /* "WAVE" */
    char fmt[4];              /* "fmt " */
    unsigned long fmt_len;    /* length of rest of fmt chunk = 16 */
    unsigned short tag;       /* MS PCM = 1 */
    unsigned short channels;  /* channels, mono = 1, stereo = 2 */
    unsigned long samp_rate;  /* samples per second = 44100 */
    unsigned long byte_rate;  /* bytes per second = samp_rate * byte_samp = 176400 */
    unsigned short byte_samp; /* block align (bytes per sample) = channels * bits_per_sample / 8 = 4 */
    unsigned short bit_samp;  /* bits per sample = 16 for MS PCM (format specific) */
    char data[4];             /* "data" */
    unsigned long length;     /* data length (bytes) */
  } header;

  if((config.wave.file = fopen(config.infile,"rb")) == NULL)
    error("Unable to open file");

  if(filetype == WAVE)
  {
    if (fread(&header, sizeof(header), 1, config.wave.file) != 1)
      error("Invalid Header");
    if(strncmp(header.riff,"RIFF",4) != 0) error("Not a MS-RIFF file");
    if(strncmp(header.wave,"WAVE",4) != 0) error("Not a WAVE audio");
    if(strncmp(header.fmt, "fmt ",4) != 0) error("Can't find format chunk");
    if(header.tag != 1)                    error("Unknown WAVE format");
    if(header.channels > 2)                error("More than 2 channels");
    if(header.bit_samp != 16)              error("Not 16 bit");
    if(strncmp(header.data,"data",4) != 0) error("Can't find data chunk");
    i = 0;
  }
  
  if(filetype == DATA)
  {
    fseek(config.wave.file, 0, SEEK_END); /* find length of file */
    header.length = ftell(config.wave.file);
    rewind(config.wave.file);
  
    if((header.length & 3) == 0) /* assume stereo if length divisable by 4 (could be mono though) */
      header.channels = 2;
    else if((header.length & 1) == 0) /* else assume mono if divisable by 2 */
      header.channels = 1;
    else error("Not a 16 bit data file");

    header.samp_rate = 44100; /* assumed */
    header.bit_samp = 16;     /* assumed */
    header.byte_samp = header.channels * header.bit_samp / 8;
    header.byte_rate = header.samp_rate * header.byte_samp;
    i = 1;
  }

  config.wave.type          = WAVE_RIFF_PCM;
  config.wave.channels      = header.channels;
  config.wave.samplerate    = header.samp_rate;
  config.wave.bits          = header.bit_samp;
  config.wave.total_samples = header.length / header.byte_samp;
  config.wave.length        = header.length / header.byte_rate;

#if !defined(SHINE_SILENT_MODE)
  printf("%s, %s %ldHz %dbit, Length: %2ld:%2ld:%2ld\n", 
          file_type[i],channel_mappings[header.channels],header.samp_rate,header.bit_samp,
          config.wave.length/3600,(config.wave.length/60)%60,config.wave.length%60);
#endif
}

/*
 * read_samples:
 * -------------
 */
int read_samples(short *sample_buffer, int frame_size)
{
  int samples_read;

  switch(config.wave.type)
  {
    case WAVE_RIFF_PCM :
      samples_read = fread(sample_buffer,sizeof(short),frame_size, config.wave.file);
  
      if(samples_read<frame_size && samples_read>0) /* Pad sample with zero's */
        while(samples_read<frame_size) sample_buffer[samples_read++] = 0;
      break;

    default:
      error("[read_samples], wave filetype not supported");
  }
  return samples_read;
}

/*
 * wave_get:
 * ---------
 * Expects an interleaved 16bit pcm stream from read_samples, which it
 * de-interleaves into buffer.
 */
int wave_get(short buffer[2][samp_per_frame])
{
  static short temp_buf[2304];
  int          samples_read;
  int          j;

  switch(config.mpeg.mode)
  {
    case MODE_MONO  :
      samples_read = read_samples(temp_buf,(int)config.mpeg.samples_per_frame);
      for(j=0;j<samp_per_frame;j++)
      {
        buffer[0][j] = temp_buf[j];
        buffer[1][j] = 0;
      }
      break;
      
    default: /* stereo */
      samples_read = read_samples(temp_buf,(int)config.mpeg.samples_per_frame<<1);
      for(j=0;j<samp_per_frame;j++)
      {
        buffer[0][j] = temp_buf[2*j];
        buffer[1][j] = temp_buf[2*j+1];
      }
  }
  return samples_read;
}
 
