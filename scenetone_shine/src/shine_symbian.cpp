/* Copyright (c) 2005, Nokia. All rights reserved */

#define SCENETONE_SHINE_IMPLEMENTATION

// INCLUDE FILES
#include <eikstart.h>
#include <e32std.h>
#include "scenetoneinterfaces.h"
#include "scenetone_shine.h"
#include <f32file.h>
#include <utf.h>

static const TInt supportedSampleRates[] = { 44100, 48000, 32000 };
static const TInt supportedBitRates[]    = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };

#define SHINE_MODE_JSTEREO     1
#define SHINE_MODE_MONO        3

config_t config;
TInt   (*callback)(TAny *aOutput, TInt aBytes);

/*
 * wave_get:
 * ---------
 * Expects an interleaved 16bit pcm stream from read_samples, which it
 * de-interleaves into buffer.
 */

// TODO: handle padding (to frame size) with the last bytes..
int wave_get(short buffer[2][samp_per_frame])
{
  static short temp_buf[2304];
  int          samples_read;
  int          j;

  switch(config.mpeg.mode)
  {
    case MODE_MONO  :
	samples_read = callback(temp_buf, (int)config.mpeg.samples_per_frame*2) / 2;
      for(j=0;j<samp_per_frame;j++)
      {
        buffer[0][j] = temp_buf[j];
        buffer[1][j] = 0;
      }


      break;
      
    default: /* stereo */
	samples_read = callback(temp_buf, (int)config.mpeg.samples_per_frame*4) / 4;
      for(j=0;j<samp_per_frame;j++)
      {
        buffer[0][j] = temp_buf[2*j];
        buffer[1][j] = temp_buf[2*j+1];
      }
  }
  return samples_read;
}


TInt CScenetoneShine::GetSupportedRates(const TInt *&aSampleRates, const TInt *&aBitRates)
{ 
    aSampleRates = supportedSampleRates;
    aBitRates    = supportedBitRates;
    return KErrNone;
}

TInt CScenetoneShine::Start(const TDesC &aOutputFileName, TInt aSampleRate, TInt aChannels, TInt aSamples, TInt aBitRate, TInt (*aCallBack)(TAny *aOutput, TInt aBytes) )
{ 
    TUint i;

    for(i=0;i<sizeof(supportedSampleRates);i++)
    {
	if(supportedSampleRates[i] == aSampleRate)
	{
	    config.mpeg.samplerate_index = i;
	    break;
	}
    }
    if(i == sizeof(supportedSampleRates)) return KErrNotSupported;

    for(i=0;i<sizeof(supportedBitRates);i++)
    {
	if(supportedBitRates[i] == aBitRate)
	{
	    config.mpeg.bitrate_index = i;
	    config.mpeg.bitr          = aBitRate;
	    break;
	}
    }
    if(i == sizeof(supportedBitRates)) return KErrNotSupported;

    callback = aCallBack;

    TBuf8<KMaxFileName+1> fname8;
    CnvUtfConverter::ConvertFromUnicodeToUtf8(fname8, aOutputFileName);
	fname8.Append(TChar('.'));
	fname8.Append(TChar('m'));
	fname8.Append(TChar('p'));
	fname8.Append(TChar('3'));
    fname8.Append(TChar('\0'));

	config.outfile = (char *)fname8.Ptr();
    config.mpeg.type = 1;
    config.mpeg.layr = 2;
    config.mpeg.psyc = 2;
    config.mpeg.emph = 0; 
    config.mpeg.crc  = 0;
    config.mpeg.ext  = 0;
    config.mpeg.mode_ext  = 0;
    config.mpeg.copyright = 0;
    config.mpeg.original  = 1;
	config.wave.channels      = aChannels;
	config.wave.samplerate    = aSampleRate;
	config.wave.bits          = 16;
	config.wave.total_samples = aSamples;	
	config.wave.length        = aSamples / aSampleRate;
	//config.wave.type         = WAVE_RIFF_PCM;
    //config.mpeg.copyright = 1;

    if( aChannels == 1 )         config.mpeg.mode = 3; //SHINE_MODE_MONO;
    else                         config.mpeg.mode = 2; //SHINE_MODE_JSTEREO;

    L3_compress();
    return KErrNone;
}

CScenetoneShine::~CScenetoneShine()
{
}

CScenetoneShine *CScenetoneShine::NewL()
{
    CScenetoneShine *p = new (ELeave) CScenetoneShine;
    p->ConstructL();
    return p;
}

EXPORT_C CScenetoneShine *ScenetoneCreateShineWriter()
{
	CScenetoneShine *p = CScenetoneShine::NewL();
	return p;
}

void CScenetoneShine::ConstructL()
{
}

