//
// /home/ms/sidplay/libsidplay/emu/RCS/mixing.cpp,v
//

//#include "mytypes.h"
//#include "opstruct.h"
#include "samples.h"
#include "6581_.h"
#include "mixing.h"


//
// this is local to the Mixer
//
#define optr1 	(iTheSidEmu->optr1)

#define optr2 	(iTheSidEmu->optr2)
#define optr3 	(iTheSidEmu->optr3)

#define voice4_gainLeft  iTheSidEmu->voice4_gainLeft
#define voice4_gainRight iTheSidEmu->voice4_gainRight


Mixer::Mixer(emuEngine* aEmuEngine, sidEmu* aSidEmu)
	:iTheSidEmu(aSidEmu)
	,signedPanMix8(0)
	,signedPanMix16(0)
	{
    CTOR(Mixer);
	sidEmuFillFunc = &Mixer::fill8bitMono; // default

	iTheSampler = iTheSidEmu->iTheSampler;
	}

Mixer::~Mixer()
	{
    DTOR(Mixer);
	iTheSampler = NULL;
	}

void Mixer::MixerInit(bool threeVoiceAmplify, ubyte zero8, uword zero16)
{
	zero8bit = zero8;
	zero16bit = zero16;
	
	long si;
	uword ui;
	
	long ampDiv = maxLogicalVoices;
	if (threeVoiceAmplify)
	{
		ampDiv = (maxLogicalVoices-1);
	}

	// Mixing formulas are optimized by sample input value.
	
	si = (-128*maxLogicalVoices);
	for (ui = 0; ui < sizeof(mix8mono); ui++ )
	{
		mix8mono[ui] = (ubyte)(si/ampDiv) + zero8bit;
		si++;
	}

	si = (-128*maxLogicalVoices);  // optimized by (/2 *2);
	for (ui = 0; ui < sizeof(mix8stereo); ui++ )
	{
		mix8stereo[ui] = (ubyte)(si/ampDiv) + zero8bit;
		si+=2;
	}

	si = (-128*maxLogicalVoices) * 256;
	for (ui = 0; ui < sizeof(mix16mono)/sizeof(uword); ui++ )
	{
		mix16mono[ui] = (uword)(si/ampDiv) + zero16bit;
		si+=256;
	}

	si = (-128*maxLogicalVoices) * 256;  // optimized by (/2 * 512)
	for (ui = 0; ui < sizeof(mix16stereo)/sizeof(uword); ui++ )
	{
		mix16stereo[ui] = (uword)(si/ampDiv) + zero16bit;
		si+=512;
	}
}


inline void Mixer::syncEm()
	{
	optr1.cycleLenCount--;
	optr2.cycleLenCount--;
	optr3.cycleLenCount--;
	bool sync1 = (optr1.modulator->cycleLenCount <= 0);
	bool sync2 = (optr2.modulator->cycleLenCount <= 0);
	bool sync3 = (optr3.modulator->cycleLenCount <= 0);
	if (optr1.sync && sync1)
	{
		optr1.cycleLenCount = 0;
		optr1.outProc = &sidEmu::waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr1.waveStep.l = 0;
#else
		optr1.waveStep = (optr1.waveStepPnt = 0);
#endif
	}
	if (optr2.sync && sync2)
	{
		optr2.cycleLenCount = 0;
		optr2.outProc = &sidEmu::waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr2.waveStep.l = 0;
#else
		optr2.waveStep = (optr2.waveStepPnt = 0);
#endif
	}
	if (optr3.sync && sync3)
	{
		optr3.cycleLenCount = 0;
		optr3.outProc = &sidEmu::waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr3.waveStep.l = 0;
#else
		optr3.waveStep = (optr3.waveStepPnt = 0);
#endif
	}
}


//
// -------------------------------------------------------------------- 8-bit
//

void* Mixer::fill8bitMono( void* buffer, udword numberOfSamples )
	{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
		{
	    *buffer8bit++ = mix8mono[(unsigned)(mix8monoMiddleIndex
											+(iTheSidEmu->*optr1.outProc)(&optr1)
											+(iTheSidEmu->*optr2.outProc)(&optr2)
											+(iTheSidEmu->*optr3.outProc)(&optr3)
											+(iTheSampler->*(iTheSampler->sampleEmuRout))())];
		syncEm();
		}
	return buffer8bit;
	}

void* Mixer::fill8bitMonoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+(iTheSidEmu->*optr1.outProc)(&optr1)]
			+signedPanMix8[optr2.gainLeft+(iTheSidEmu->*optr2.outProc)(&optr2)]
			+signedPanMix8[optr3.gainLeft+(iTheSidEmu->*optr3.outProc)(&optr3)]
			+signedPanMix8[voice4_gainLeft+(iTheSampler->*(iTheSampler->sampleEmuRout))()];
		syncEm();
	}
	return buffer8bit;
}

void* Mixer::fill8bitStereo( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
  	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(iTheSidEmu->*optr1.outProc)(&optr1)
											 +(iTheSidEmu->*optr3.outProc)(&optr3))];
		// right
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(iTheSidEmu->*optr2.outProc)(&optr2)
											 +(iTheSampler->*(iTheSampler->sampleEmuRout))())];
		syncEm();
	}
	return buffer8bit;
}

void* Mixer::fill8bitStereoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (iTheSidEmu->*optr1.outProc)(&optr1);
		voice2data = (iTheSidEmu->*optr2.outProc)(&optr2);
		voice3data = (iTheSidEmu->*optr3.outProc)(&optr3);
		voice4data = (iTheSampler->*(iTheSampler->sampleEmuRout))();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
		    +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainRight+voice1data]
			+signedPanMix8[optr2.gainRight+voice2data]
			+signedPanMix8[optr3.gainRight+voice3data]
			+signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* Mixer::fill8bitStereoSurround( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (iTheSidEmu->*optr1.outProc)(&optr1);
		voice2data = (iTheSidEmu->*optr2.outProc)(&optr2);
		voice3data = (iTheSidEmu->*optr3.outProc)(&optr3);
		voice4data = (iTheSampler->*(iTheSampler->sampleEmuRout))();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
		    +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			-signedPanMix8[optr1.gainRight+voice1data]
			-signedPanMix8[optr2.gainRight+voice2data]
			-signedPanMix8[optr3.gainRight+voice3data]
			-signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* Mixer::fill8bitsplit( void* buffer, udword numberOfSamples )
{
	ubyte* v1buffer8bit = (ubyte*)buffer;
	ubyte* v2buffer8bit = v1buffer8bit + iTheSidEmu->splitBufferLen;
	ubyte* v3buffer8bit = v2buffer8bit + iTheSidEmu->splitBufferLen;
	ubyte* v4buffer8bit = v3buffer8bit + iTheSidEmu->splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer8bit++ = zero8bit+(iTheSidEmu->*optr1.outProc)(&optr1);
		*v2buffer8bit++ = zero8bit+(iTheSidEmu->*optr2.outProc)(&optr2);
		*v3buffer8bit++ = zero8bit+(iTheSidEmu->*optr3.outProc)(&optr3);
		*v4buffer8bit++ = zero8bit+(iTheSampler->*(iTheSampler->sampleEmuRout))();
		syncEm();
	}
	return v1buffer8bit;
}

//
// ------------------------------------------------------------------- 16-bit
//

void* Mixer::fill16bitMono( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer16bit++ = mix16mono[(unsigned)(mix16monoMiddleIndex
											 +(iTheSidEmu->*optr1.outProc)(&optr1)
											 +(iTheSidEmu->*optr2.outProc)(&optr2)
											 +(iTheSidEmu->*optr3.outProc)(&optr3)
											 +(iTheSampler->*(iTheSampler->sampleEmuRout))())];
		syncEm();
	}
	return buffer16bit;
}

void* Mixer::fill16bitMonoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+(iTheSidEmu->*optr1.outProc)(&optr1)]
			+signedPanMix16[optr2.gainLeft+(iTheSidEmu->*optr2.outProc)(&optr2)]
			+signedPanMix16[optr3.gainLeft+(iTheSidEmu->*optr3.outProc)(&optr3)]
			+signedPanMix16[voice4_gainLeft+(iTheSampler->*(iTheSampler->sampleEmuRout))()];
		syncEm();
	}
	return buffer16bit;
}

void* Mixer::fill16bitStereo( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
											 +(iTheSidEmu->*optr1.outProc)(&optr1)
											 +(iTheSidEmu->*optr3.outProc)(&optr3))];
		// right
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
											 +(iTheSidEmu->*optr2.outProc)(&optr2)
											 +(iTheSampler->*(iTheSampler->sampleEmuRout))())];
		syncEm();
	}
	return buffer16bit;
}

void* Mixer::fill16bitStereoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (iTheSidEmu->*optr1.outProc)(&optr1);
		voice2data = (iTheSidEmu->*optr2.outProc)(&optr2);
		voice3data = (iTheSidEmu->*optr3.outProc)(&optr3);
		voice4data = (iTheSampler->*(iTheSampler->sampleEmuRout))();
		// left
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+voice1data]
		    +signedPanMix16[optr2.gainLeft+voice2data]
			+signedPanMix16[optr3.gainLeft+voice3data]
			+signedPanMix16[voice4_gainLeft+voice4data];
		// right
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainRight+voice1data]
			+signedPanMix16[optr2.gainRight+voice2data]
			+signedPanMix16[optr3.gainRight+voice3data]
			+signedPanMix16[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer16bit;
}

void* Mixer::fill16bitStereoSurround( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (iTheSidEmu->*optr1.outProc)(&optr1);
		voice2data = (iTheSidEmu->*optr2.outProc)(&optr2);
		voice3data = (iTheSidEmu->*optr3.outProc)(&optr3);
		voice4data = (iTheSampler->*(iTheSampler->sampleEmuRout))();
		// left
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+voice1data]
		    +signedPanMix16[optr2.gainLeft+voice2data]
			+signedPanMix16[optr3.gainLeft+voice3data]
			+signedPanMix16[voice4_gainLeft+voice4data];
		// right
		*buffer16bit++ = zero16bit
			-signedPanMix16[optr1.gainRight+voice1data]
			-signedPanMix16[optr2.gainRight+voice2data]
			-signedPanMix16[optr3.gainRight+voice3data]
			-signedPanMix16[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer16bit;
}

void* Mixer::fill16bitsplit( void* buffer, udword numberOfSamples )
{
	sword* v1buffer16bit = (sword*)buffer;
	sword* v2buffer16bit = v1buffer16bit + iTheSidEmu->splitBufferLen;
	sword* v3buffer16bit = v2buffer16bit + iTheSidEmu->splitBufferLen;
	sword* v4buffer16bit = v3buffer16bit + iTheSidEmu->splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer16bit++ = zero16bit+(iTheSidEmu->*optr1.outProc)(&optr1);
		*v2buffer16bit++ = zero16bit+(iTheSidEmu->*optr2.outProc)(&optr2);
		*v3buffer16bit++ = zero16bit+(iTheSidEmu->*optr3.outProc)(&optr3);
		*v4buffer16bit++ = zero16bit+(iTheSampler->*(iTheSampler->sampleEmuRout))();
		syncEm();
	}
	return v1buffer16bit;
}
