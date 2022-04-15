/* layer3.c */
 
#include "Types.h"
#include "Wave.h"
#include "Layer3.h"
#include "L3SubBand.h"
#include "L3mdct.h"
#include "L3Loop.h"
#include "Bitstream.h"
#include "L3Bitstrea.h"

/*
 * update_status:
 * --------------
 */
static void update_status(int frames_processed)
{
#if !defined(SHINE_SILENT_MODE)
  printf("\015[Frame %6d of %6ld] (%2.2f%%)", 
            frames_processed,config.mpeg.total_frames,
            (double)((double)frames_processed/config.mpeg.total_frames)*100); 
#endif
  fflush(stdout);
}

/*
 * L3_compress:
 * ------------
 */
void L3_compress(void)
{
  int             frames_processed;
  int             channel;
  int             i;
  int             gr;
  double          pe[2][2];
  short          *buffer_window[2];
  double          avg_slots_per_frame;
  double          frac_slots_per_frame;
  long            whole_slots_per_frame;
  double          slot_lag;
  int             mean_bits;
  int             sideinfo_len;
  static short    buffer[2][samp_per_frame];
  static int      l3_enc[2][2][samp_per_frame2];
  static long     l3_sb_sample[2][3][18][SBLIMIT];
  static long     mdct_freq[2][2][samp_per_frame2];
  static L3_psy_ratio_t  ratio;
  static L3_side_info_t  side_info;
  static L3_scalefac_t   scalefactor;
  static bitstream_t     bs;

  open_bit_stream_w(&bs, config.outfile, BUFFER_SIZE);
  
  memset((char *)&side_info,0,sizeof(L3_side_info_t));

  L3_subband_initialise();
  L3_mdct_initialise();
  L3_loop_initialise();
  
  config.mpeg.samples_per_frame = samp_per_frame;
  config.mpeg.total_frames      = config.wave.total_samples/config.mpeg.samples_per_frame;
  config.mpeg.bits_per_slot     = 8;
  frames_processed              = 0;
  sideinfo_len = (config.wave.channels==1) ? 168 : 288;

  /* Figure average number of 'slots' per frame. */
  avg_slots_per_frame   = ((double)config.mpeg.samples_per_frame / 
                           ((double)config.wave.samplerate/1000)) *
                          ((double)config.mpeg.bitr /
                           (double)config.mpeg.bits_per_slot);
  whole_slots_per_frame = (int)avg_slots_per_frame;
  frac_slots_per_frame  = avg_slots_per_frame - (double)whole_slots_per_frame;
  slot_lag              = -frac_slots_per_frame;
  if(frac_slots_per_frame==0)
    config.mpeg.padding = 0;

  while(wave_get(buffer))
  {
    update_status(++frames_processed);

    buffer_window[0] = buffer[0];
    buffer_window[1] = buffer[1];

    if(frac_slots_per_frame)
      if(slot_lag>(frac_slots_per_frame-1.0))
      { /* No padding for this frame */
        slot_lag    -= frac_slots_per_frame;
        config.mpeg.padding = 0;
      }
      else 
      { /* Padding for this frame  */
        slot_lag    += (1-frac_slots_per_frame);
        config.mpeg.padding = 1;
      }
      
    config.mpeg.bits_per_frame = 8*(whole_slots_per_frame + config.mpeg.padding);
    mean_bits = (config.mpeg.bits_per_frame - sideinfo_len)>>1;

    /* polyphase filtering */
    for(gr=0;gr<2;gr++)
      for(channel=config.wave.channels; channel--; )
        for(i=0;i<18;i++)
          L3_window_filter_subband(&buffer_window[channel], &l3_sb_sample[channel][gr+1][i][0] ,channel);

    /* apply mdct to the polyphase output */
    L3_mdct_sub(l3_sb_sample, mdct_freq);

    /* bit and noise allocation */
    L3_iteration_loop(pe,mdct_freq,&ratio,&side_info, l3_enc, mean_bits,&scalefactor);

    /* write the frame to the bitstream */
    L3_format_bitstream(l3_enc,&side_info,&scalefactor, &bs,mdct_freq,NULL,0);

  }    
  close_bit_stream(&bs);
}


