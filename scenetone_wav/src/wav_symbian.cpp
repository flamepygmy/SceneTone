//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006  Jani Vaarala <flame@pygmyprojects.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  --------------------------------------------------------------------------

// INCLUDE FILES
#include <eikstart.h>
#include <e32std.h>
#include "scenetoneinterfaces.h"
#include "scenetone_wav.h"
#include <f32file.h>
#include <utf.h>

#define WAV_GEN_BUF_SIZE		  8192

#define WAV_LEN_OF_FILE_MINUS_8   4       // 32-bit
#define WAV_CHANNEL_COUNT         10+12   // 16-bit
#define WAV_SAMPLE_RATE           12+12   // 32-bit
#define WAV_BYTES_PER_SECOND      16+12   // 32-bit
#define WAV_BYTES_PER_SAMPLE      20+12   // 16-bit ?
#define WAV_BITS_PER_SAMPLE       22+12   // 16-bit
#define WAV_LEN_OF_SAMPLE_DATA    4+36    // 32-bit

#define EMIT32(p,a) {(p)[0] = (unsigned char)((a)&0xff); (p)[1] = (unsigned char)(((a) >> 8)&0xff); (p)[2] = (unsigned char)(((a) >> 16)&0xff); (p)[3] = (unsigned char)(((a) >> 24)&0xff); }
#define EMIT16(p,a) {(p)[0] = (unsigned char)((a)&0xff); (p)[1] = (unsigned char)(((a) >> 8)&0xff);}

const unsigned char header_44100_stereo[] =
{
    0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
};

const int supportedSampleRates[] = { 22050, 44100, -1 };
const int supportedBitRates[]    = { 11111, -1 };

// samples == amount of "mono samples"
void patch_wav_header(int channels, int sample_rate, int samples, unsigned char *header)
{
    int sampledatalen = samples * channels * 2;
    int totlen        = sampledatalen + sizeof(header_44100_stereo) - 8;

    EMIT32(&header[WAV_LEN_OF_FILE_MINUS_8], totlen);
    EMIT16(&header[WAV_CHANNEL_COUNT],       channels);
    EMIT32(&header[WAV_SAMPLE_RATE],         sample_rate);
    EMIT32(&header[WAV_BYTES_PER_SECOND],    sample_rate*channels*2);
    EMIT16(&header[WAV_BYTES_PER_SAMPLE],    channels*2);
    EMIT16(&header[WAV_BITS_PER_SAMPLE],     channels*16);
    EMIT32(&header[WAV_LEN_OF_SAMPLE_DATA],  sampledatalen);
}

TInt CScenetoneWav::GetSupportedRates(const TInt *&aSampleRates, const TInt *&aBitRates)
{ 
    aSampleRates = supportedSampleRates;
    aBitRates    = supportedBitRates;
    return KErrNone;
}

TInt CScenetoneWav::Start(const TDesC &aOutputFileName, TInt aSampleRate, TInt aChannels, TInt aSamples, TInt aBitRate, TInt (*aCallBack)(TAny *aOutput, TInt aBytes) )
{ 
    TUint i;

    for(i=0;i<sizeof(supportedSampleRates);i++)
    {
		if(supportedSampleRates[i] == aSampleRate)
		{
	    	break;
		}
    }
    if(i == sizeof(supportedSampleRates)) return KErrNotSupported;

	TBuf<KMaxFileName+1> fname = aOutputFileName;
	fname.Append(TChar('.'));
	fname.Append(TChar('w'));
	fname.Append(TChar('a'));
	fname.Append(TChar('v'));

    TUint bytes = aSamples * aChannels * 2;

    RFs session;
    session.Connect();
    RFile f;
    unsigned char *buffer = new (ELeave) unsigned char [WAV_GEN_BUF_SIZE];
	TPtr8    desc = TPtr8(NULL,0);

    f.Replace( session, fname, EFileWrite );
    
    unsigned char *hdr = new (ELeave) unsigned char[sizeof(header_44100_stereo)];
    Mem::Copy(hdr, header_44100_stereo, sizeof(header_44100_stereo));
    
    patch_wav_header( aChannels, aSampleRate, aSamples, hdr );
    desc.Set(hdr, sizeof(header_44100_stereo), sizeof(header_44100_stereo));
    f.Write( desc, sizeof(header_44100_stereo) );

	while(1)
	{
		if(bytes == 0)	break;

		if(bytes < WAV_GEN_BUF_SIZE)
		{
			aCallBack(buffer, bytes);
			desc.Set(buffer, bytes, bytes);
		 	f.Write( desc, bytes );
			break;
		}
		else
		{
			aCallBack(buffer, WAV_GEN_BUF_SIZE);
			desc.Set(buffer, WAV_GEN_BUF_SIZE, WAV_GEN_BUF_SIZE);
		 	f.Write( desc, WAV_GEN_BUF_SIZE );
			bytes -= WAV_GEN_BUF_SIZE;
		}
	}
	
	f.Close();
	session.Close();
	
	delete buffer;
	delete hdr;
	
    return KErrNone;
}

CScenetoneWav::~CScenetoneWav()
{
}

CScenetoneWav *CScenetoneWav::NewL()
{
    CScenetoneWav *p = new (ELeave) CScenetoneWav;
    p->ConstructL();
    return p;
}

EXPORT_C CScenetoneWav *ScenetoneCreateWavWriter()
{
	CScenetoneWav *p = CScenetoneWav::NewL();
	return p;
}

void CScenetoneWav::ConstructL()
{
}

