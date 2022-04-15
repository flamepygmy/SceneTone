//
// 1997/05/11 11:29:47
//

#ifndef __6581_SAMPLES_H
  #define __6581_SAMPLES_H


#include "mytypes.h"
#include "myendian.h"

class sidEmu;
class C6510;



struct sampleChannel
{
	bool Active;
	char Mode;
	ubyte Counter;  // Galway
	ubyte Repeat;
	ubyte Scale;
	ubyte SampleOrder;
	sbyte VolShift;
	
	uword Address;
	uword EndAddr;
	uword RepAddr;
	
	uword SamAddr;  // Galway
	uword SamLen;
	uword LoopWait;
	uword NullWait;
	
	uword Period;
#if defined(DIRECT_FIXPOINT) 
	cpuLword Period_stp;
	cpuLword Pos_stp;
	cpuLword PosAdd_stp;
#elif defined(PORTABLE_FIXPOINT)
	uword Period_stp, Period_pnt;
	uword Pos_stp, Pos_pnt;
	uword PosAdd_stp, PosAdd_pnt;
#else
	udword Period_stp;
	udword Pos_stp;
	udword PosAdd_stp;
#endif
};

class Mixer;



/**
 * class Sample - handles all the sampling
 */
class Sample
	{

	friend class Mixer;
	
enum
{
	FM_NONE,
	FM_GALWAYON,
	FM_GALWAYOFF,
	FM_HUELSON,
	FM_HUELSOFF
};
public:
	Sample(C6510* a6510);
	~Sample();
	void sampleEmuInit(sidEmu* aTheSidEmu);
	void sampleEmuCheckForInit();
	void sampleEmuReset(sidEmu* aTheSidEmu);

private:
	void channelReset(sampleChannel& ch);
	inline void channelFree(sampleChannel& ch, const uword regBase);
	inline void channelTryInit(sampleChannel& ch, const uword regBase);
	inline ubyte channelProcess(sampleChannel& ch, const uword regBase);
	
	inline void sampleEmuTryStopAll();
	sbyte sampleEmuSilence();
	sbyte sampleEmu();
	sbyte sampleEmuTwo();

	void  GalwayInit();
	sbyte GalwayReturnSample();
	inline void GetNextFour();
//void SelectVolume(void); TODO

  private:
	udword sampleClock;
	sampleChannel ch4, ch5;
	ubyte galwayNoiseTab2[64*16];

	sbyte (Sample::*sampleEmuRout)(); ///< function pointer to the sampler routine

	ubyte* c64mem1;       ///< pointer to 64KB C64-RAM owned by C6510
	ubyte* c64mem2;       ///< pointer to Basic-ROM, VIC, SID, I/O, Kernal-ROM owned by C6510
	};


#endif // __6581_SAMPLES_H
