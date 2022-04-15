//
// 1997/09/27 21:32:09
// 
//=========================================================================
// This source implements the ADSR volume envelope of the SID-chip.
// Two different envelope shapes are implemented, an exponential
// approximation and the linear shape, which can easily be determined 
// by reading the registers of the third SID operator.
//
// Accurate volume envelope times as of November 1994 are used 
// courtesy of George W. Taylor <aa601@cfn.cs.dal.ca>, <yurik@io.org>
// They are slightly modified.
// 
// To use the rounded envelope times from the C64 Programmers Reference
// Book define SID_REFTIMES at the Makefile level.
// 
// To perform realtime calculations with floating point precision define
// SID_FPUENVE at the Makefile level. On high-end FPUs (not Pentium !),
// this can result in speed improvement. Default is integer fixpoint.
// 
// Global Makefile definables:
//
//   DIRECT_FIXPOINT - use a union to access integer fixpoint operands 
//                     in memory. This makes an assumption about the
//                     hardware and software architecture and therefore 
//                     is considered a hack !
//
// Local (or Makefile) definables:
// 
//   SID_LINENVE  - use linear envelope attack shape (default)
//   SID_EXPENVE  - use exponential envelope shape
//   SID_REFTIMES - use rounded envelope times
//   SID_FPUENVE  - use floating point precision for calculations
//                  (will override the global DIRECT_FIXPOINT setting !)
//
//=========================================================================

#if defined(SID_EXPENVE) && defined(SID_LINENVE)
  #error Select either SID_LINENVE or SID_EXPENVE !
#elif !defined(SID_EXPENVE) && !defined(SID_LINENVE)
  #define SID_LINENVE 1
#endif

#include <math.h>

#include "envelope.h"
//#include "myendian.h"
#if defined(SID_EXPENVE)
  #include "enve_ac.h"
  #include "enve_dc.h"
#elif defined(SID_LINENVE)
  #include "enve_dl.h"
#endif


//
// const data
//
extern const ubyte masterVolumeLevels[16] =
{
    0,  17,  34,  51,  68,  85, 102, 119,
  136, 153, 170, 187, 204, 221, 238, 255
};

static const float attackTimes[16] =
{
  // milliseconds
#if defined(SID_REFTIMES)
  2,8,16,24,38,56,68,80,
  100,250,500,800,1000,3000,5000,8000
#else
  2.2528606f, 8.0099577f, 15.7696042f, 23.7795619f, 37.2963655f, 55.0684591f,
  66.8330845f, 78.3473987f,
  98.1219818f, 244.554021f, 489.108042f, 782.472742f, 977.715461f, 2933.64701f,
  4889.07793f, 7822.72493f
#endif
};

static const float decayReleaseTimes[16] =
{
  // milliseconds
#if defined(SID_REFTIMES)
  8,24,48,72,114,168,204,240,
  300,750,1500,2400,3000,9000,15000,24000
#else
  8.91777693f, 24.594051f, 48.4185907f, 73.0116639f, 114.512475f, 169.078356f,
  205.199432f, 240.551975f, 
  301.266125f, 750.858245f, 1501.71551f, 2402.43682f, 3001.89298f, 9007.21405f,
  15010.998f, 24018.2111f
#endif
};


//
// implementation of Envelope
//

Envelope::Envelope()
/**
 * C'tor
 */
	{
	CTOR(Envelope);
	}

Envelope::~Envelope()
/**
 * D'tor
 */
	{
	DTOR(Envelope);
	}

void Envelope::enveEmuInit( udword updateFreq, bool measuredValues )
{
	udword i, j, k;
  
#if !defined(SID_LINENVE) && defined(SID_EXPENVE)
	attackTabLen = sizeof(attackTab);
	for ( i = 0; i < 256; i++ )
	{
		j = 0;
		while (( j < attackTabLen ) && (attackTab[j] < i) )
		{
			j++;
		}
		attackPos[i]=j;
	}
#endif

	releaseTabLen = sizeof(releaseTab);
	for ( i = 0; i < 256; i++ )
	{
		j = 0;
		while (( j < releaseTabLen ) && (releaseTab[j] > i) )
		{
			j++;
		}
		if ( j < releaseTabLen )
		{
			releasePos[i] = j;
		}
		else
		{
			releasePos[i] = releaseTabLen -1;
		}
	}

	k = 0;
	for ( i = 0; i < 16; i++ )
	{
		for ( j = 0; j < 256; j++ )
		{
			uword tmpVol = (uword)j;
			if (measuredValues)
			{
				tmpVol = (sword) ((293.0*(1-exp(j/-130.0)))+4.0); //ALFRED - TODO: check this, possible bug
				if (j == 0)
					tmpVol = 0;
				if (tmpVol > 255)
					tmpVol = 255;
			}
			// Want the modulated volume value in the high byte.
			masterAmplModTable[k++] = ((tmpVol * masterVolumeLevels[i]) / 255) << 8;
		}
	}
	
	for ( i = 0; i < 16; i++ )
	{
#ifdef SID_FPUENVE
		double scaledenvelen = floor(( attackTimes[i] * updateFreq ) / 1000UL );
		if (scaledenvelen == 0)
			scaledenvelen = 1;
		attackRates[i] = attackTabLen / scaledenvelen;

		scaledenvelen = floor(( decayReleaseTimes[i] * updateFreq ) / 1000UL );
		if (scaledenvelen == 0)
			scaledenvelen = 1;
		decayReleaseRates[i] = releaseTabLen / scaledenvelen;
#elif defined(DIRECT_FIXPOINT)

		udword scaledenvelen = (sdword) floor( (attackTimes[i]*updateFreq) / 1000UL );

		if (scaledenvelen == 0)
			scaledenvelen = 1;
		attackRates[i] = (attackTabLen << 16) / scaledenvelen;

		scaledenvelen = (sdword)floor(( decayReleaseTimes[i] * updateFreq ) / 1000UL );
		if (scaledenvelen == 0)
			scaledenvelen = 1;
		decayReleaseRates[i] = (releaseTabLen << 16) / scaledenvelen;
#else
		udword scaledenvelen = (sdword)floor(( attackTimes[i] * updateFreq ) / 1000UL ); //ALFRED - TODO: check this, possible bug

		if (scaledenvelen == 0)
			scaledenvelen = 1;
		attackRates[i] = attackTabLen / scaledenvelen;
		attackRatesP[i] = (( attackTabLen % scaledenvelen ) * 65536UL ) / scaledenvelen;
		
		scaledenvelen = (sdword)floor(( decayReleaseTimes[i] * updateFreq ) / 1000UL ); //ALFRED - TODO: check this, possible bug
		if (scaledenvelen == 0)
			scaledenvelen = 1;
		decayReleaseRates[i] = releaseTabLen / scaledenvelen;
		decayReleaseRatesP[i] = (( releaseTabLen % scaledenvelen ) * 65536UL ) / scaledenvelen;
#endif
  }
}


void Envelope::enveEmuResetOperator(struct sidOperator* pVoice)
/**
 * Reset operator
 */
	{
	// mute, end of R-phase
	pVoice->ADSRctrl = ENVE_MUTE;
	pVoice->gateOnCtrl = (pVoice->gateOffCtrl = false);
	
#ifdef SID_FPUENVE
	pVoice->fenveStep = (pVoice->fenveStepAdd = 0);
	pVoice->enveStep = 0;
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStep.l = (pVoice->enveStepAdd.l = 0);
#else
	pVoice->enveStep = (pVoice->enveStepPnt = 0);
	pVoice->enveStepAdd = (pVoice->enveStepAddPnt = 0);
#endif
	pVoice->enveSusVol = 0;
	pVoice->enveVol = 0;
	pVoice->enveShortAttackCount = 0;
}


const ptr2sidUwordFunc* Envelope::getEnveModeTable() const
	{
	return enveModeTable;
	}

//
// jump tables
//
const ptr2sidUwordFunc Envelope::enveModeTable[32] =
{
// 0
 &Envelope::enveEmuStartAttack, &Envelope::enveEmuStartRelease,
   &Envelope::enveEmuAttack, &Envelope::enveEmuDecay,
   &Envelope::enveEmuSustain, &Envelope::enveEmuRelease,
   &Envelope::enveEmuSustainDecay, &Envelope::enveEmuMute,

   // 16
   &Envelope::enveEmuStartShortAttack, &Envelope::enveEmuMute,
   &Envelope::enveEmuMute, &Envelope::enveEmuMute,
   &Envelope::enveEmuMute, &Envelope::enveEmuMute,
   &Envelope::enveEmuMute, &Envelope::enveEmuMute,

   // 32
   &Envelope::enveEmuStartAttack, &Envelope::enveEmuStartRelease,
   &Envelope::enveEmuAlterAttack, &Envelope::enveEmuAlterDecay,
   &Envelope::enveEmuAlterSustain, &Envelope::enveEmuAlterRelease,
   &Envelope::enveEmuAlterSustainDecay, &Envelope::enveEmuMute,

   // 48
   &Envelope::enveEmuStartShortAttack, &Envelope::enveEmuMute,
   &Envelope::enveEmuMute, &Envelope::enveEmuMute, 
   &Envelope::enveEmuMute, &Envelope::enveEmuMute,
   &Envelope::enveEmuMute, &Envelope::enveEmuMute
};

// Real-time functions.
// Order is important because of inline optimizations.
//
// ADSRctrl is (index*2) to enveModeTable[], because of KEY-bit.

inline void enveEmuEnveAdvance(struct sidOperator* pVoice)
{
#ifdef SID_FPUENVE
	pVoice->fenveStep += pVoice->fenveStepAdd;
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStep.l += pVoice->enveStepAdd.l;
#else
	pVoice->enveStepPnt += pVoice->enveStepAddPnt;
	pVoice->enveStep += pVoice->enveStepAdd + ( pVoice->enveStepPnt > 65535 );
	pVoice->enveStepPnt &= 0xFFFF;
#endif
}

//
// Mute/Idle.
//

// Only used in the beginning.
inline uword Envelope::enveEmuMute(struct sidOperator* /*pVoice*/) const
{
	return 0;
}

//
// Release
//

inline uword Envelope::enveEmuRelease(struct sidOperator* pVoice) const
{
#ifdef SID_FPUENVE
	pVoice->enveStep = (uword)pVoice->fenveStep;
#endif
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
	if ( pVoice->enveStep.w[HI] >= releaseTabLen )
#else
	if ( pVoice->enveStep >= releaseTabLen )
#endif
	{
		pVoice->enveVol = releaseTab[releaseTabLen -1];
		return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
	}	
	else
	{
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
		pVoice->enveVol = releaseTab[pVoice->enveStep.w[HI]];
#else
		pVoice->enveVol = releaseTab[pVoice->enveStep];
#endif
		enveEmuEnveAdvance(pVoice);
		return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
	}
}


// This is the same as enveEmuStartSustainDecay().
inline uword Envelope::enveEmuAlterSustainDecay(struct sidOperator* pVoice) const
{
	ubyte decay = pVoice->SIDAD & 0x0F ;
#ifdef SID_FPUENVE
	pVoice->fenveStepAdd = decayReleaseRates[decay];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStepAdd.l = decayReleaseRates[decay];
#else
	pVoice->enveStepAdd = decayReleaseRates[decay];
	pVoice->enveStepAddPnt = decayReleaseRatesP[decay];
#endif
	pVoice->ADSRproc = &Envelope::enveEmuSustainDecay;
	return enveEmuSustainDecay(pVoice);
}


inline uword Envelope::enveEmuAlterRelease(struct sidOperator* pVoice) const
{
	ubyte release = pVoice->SIDSR & 0x0F;
#ifdef SID_FPUENVE
	pVoice->fenveStepAdd = decayReleaseRates[release];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStepAdd.l = decayReleaseRates[release];
#else
	pVoice->enveStepAdd = decayReleaseRates[release];
	pVoice->enveStepAddPnt = decayReleaseRatesP[release];
#endif
	pVoice->ADSRproc = &Envelope::enveEmuRelease;
	return enveEmuRelease(pVoice);
}

inline uword Envelope::enveEmuStartRelease(struct sidOperator* pVoice) const
{
	pVoice->ADSRctrl = ENVE_RELEASE;
#ifdef SID_FPUENVE
	pVoice->fenveStep = releasePos[pVoice->enveVol];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStep.w[HI] = (uword)releasePos[pVoice->enveVol];
	pVoice->enveStep.w[LO] = 0;
#else
	pVoice->enveStep = releasePos[pVoice->enveVol];
	pVoice->enveStepPnt = 0;
#endif
	return enveEmuAlterRelease(pVoice);
}

//
// Sustain
//

inline uword Envelope::enveEmuSustain(struct sidOperator* pVoice) const
{
	return masterAmplModTable[masterVolumeAmplIndex+pVoice->enveVol];
}

inline uword Envelope::enveEmuSustainDecay(struct sidOperator* pVoice) const
{
#ifdef SID_FPUENVE
	pVoice->enveStep = (uword)pVoice->fenveStep;
#endif
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
	if ( pVoice->enveStep.w[HI] >= releaseTabLen )
#else
	if ( pVoice->enveStep >= releaseTabLen )
#endif
	{
		pVoice->enveVol = releaseTab[releaseTabLen-1];
		return enveEmuAlterSustain(pVoice);
	}
	else
	{
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
		pVoice->enveVol = releaseTab[pVoice->enveStep.w[HI]];
#else
		pVoice->enveVol = releaseTab[pVoice->enveStep];
#endif
		// Will be controlled from sidEmuSet2().
		if ( pVoice->enveVol <= pVoice->enveSusVol )
		{
			pVoice->enveVol = pVoice->enveSusVol;
			return enveEmuAlterSustain(pVoice);
		}
		else
		{
			enveEmuEnveAdvance(pVoice);
			return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
		}
	}
}


// This is the same as enveEmuStartSustain().
inline uword Envelope::enveEmuAlterSustain(struct sidOperator* pVoice) const
{
	if ( pVoice->enveVol > pVoice->enveSusVol )
	{
		pVoice->ADSRctrl = ENVE_SUSTAINDECAY;
		pVoice->ADSRproc = &Envelope::enveEmuSustainDecay;
		return enveEmuAlterSustainDecay(pVoice);
	}
	else
	{
		pVoice->ADSRctrl = ENVE_SUSTAIN;
		pVoice->ADSRproc = &Envelope::enveEmuSustain;
		return enveEmuSustain(pVoice);
	}
}

//
// Decay
//

inline uword Envelope::enveEmuDecay(struct sidOperator* pVoice) const
{
#ifdef SID_FPUENVE
	pVoice->enveStep = (uword)pVoice->fenveStep;
#endif
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
	if ( pVoice->enveStep.w[HI] >= releaseTabLen )
#else
	if ( pVoice->enveStep >= releaseTabLen )
#endif
	{
		pVoice->enveVol = pVoice->enveSusVol;
		return enveEmuAlterSustain(pVoice);  // start sustain
	}
	else
	{
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
		pVoice->enveVol = releaseTab[pVoice->enveStep.w[HI]];
#else
		pVoice->enveVol = releaseTab[pVoice->enveStep];
#endif
		// Will be controlled from sidEmuSet2().
		if ( pVoice->enveVol <= pVoice->enveSusVol )
		{
			pVoice->enveVol = pVoice->enveSusVol;
			return enveEmuAlterSustain(pVoice);  // start sustain
		}
		else
		{
			enveEmuEnveAdvance(pVoice);
			return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
		}
	}
}

inline uword Envelope::enveEmuAlterDecay(struct sidOperator* pVoice) const
{
	ubyte decay = pVoice->SIDAD & 0x0F ;
#ifdef SID_FPUENVE
	pVoice->fenveStepAdd = decayReleaseRates[decay];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStepAdd.l = decayReleaseRates[decay];
#else
	pVoice->enveStepAdd = decayReleaseRates[decay];
	pVoice->enveStepAddPnt = decayReleaseRatesP[decay];
#endif
	pVoice->ADSRproc = &Envelope::enveEmuDecay;
	return enveEmuDecay(pVoice);
}

inline uword Envelope::enveEmuStartDecay(struct sidOperator* pVoice) const
{
	pVoice->ADSRctrl = ENVE_DECAY;
#ifdef SID_FPUENVE
	pVoice->fenveStep = 0;
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStep.l = 0;
#else
	pVoice->enveStep = (pVoice->enveStepPnt = 0);
#endif
	return enveEmuAlterDecay(pVoice);
}

//
// Attack
//

inline uword Envelope::enveEmuAttack(struct sidOperator* pVoice) const
{
#ifdef SID_FPUENVE
	pVoice->enveStep = (uword)pVoice->fenveStep;
#endif
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
  #if defined(SID_LINENVE)
	if ( pVoice->enveStep.w[HI] > attackTabLen )
  #else
	if ( pVoice->enveStep.w[HI] >= attackTabLen )
  #endif
#else
	if ( pVoice->enveStep >= attackTabLen )
#endif
		return enveEmuStartDecay(pVoice);
	else
	{
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
  #if defined(SID_LINENVE)
		pVoice->enveVol = (ubyte)pVoice->enveStep.w[HI];
  #else
		pVoice->enveVol = attackTab[pVoice->enveStep.w[HI]];
  #endif
#else
  #if defined(SID_LINENVE)
		pVoice->enveVol = pVoice->enveStep;
  #else
		pVoice->enveVol = attackTab[pVoice->enveStep];
  #endif
#endif
		enveEmuEnveAdvance(pVoice);
		return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
	}
}

inline uword Envelope::enveEmuAlterAttack(struct sidOperator* pVoice) const
{
	ubyte attack = pVoice->SIDAD >> 4;
#ifdef SID_FPUENVE
	pVoice->fenveStepAdd = attackRates[attack];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStepAdd.l = attackRates[attack];
#else
	pVoice->enveStepAdd = attackRates[attack];
	pVoice->enveStepAddPnt = attackRatesP[attack];
#endif
	pVoice->ADSRproc = &Envelope::enveEmuAttack;
	return enveEmuAttack(pVoice);
}

inline uword Envelope::enveEmuStartAttack(struct sidOperator* pVoice) const
{
	pVoice->ADSRctrl = ENVE_ATTACK;
#ifdef SID_FPUENVE
  #if defined(SID_LINENVE)
	pVoice->fenveStep = (float)pVoice->enveVol;
  #else
	pVoice->fenveStep = attackPos[pVoice->enveVol];
  #endif
#elif defined(DIRECT_FIXPOINT)
  #if defined(SID_LINENVE)
	pVoice->enveStep.w[HI] = pVoice->enveVol;
  #else
	pVoice->enveStep.w[HI] = attackPos[pVoice->enveVol];
  #endif
	pVoice->enveStep.w[LO] = 0;
#else
  #if defined(SID_LINENVE)
	pVoice->enveStep = pVoice->enveVol;
  #else
	pVoice->enveStep = attackPos[pVoice->enveVol];
  #endif
	pVoice->enveStepPnt = 0;
#endif
	return enveEmuAlterAttack(pVoice);
}

//
// Experimental.
//

//#include <iostream.h>
//#include <iomanip.h>

inline uword Envelope::enveEmuShortAttack(struct sidOperator* pVoice) const
{
#ifdef SID_FPUENVE
	pVoice->enveStep = (uword)pVoice->fenveStep;
#endif
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
  #if defined(SID_LINENVE)
	if ((pVoice->enveStep.w[HI] > attackTabLen) ||
		(pVoice->enveShortAttackCount == 0))
  #else
	if ((pVoice->enveStep.w[HI] >= attackTabLen) ||
		(pVoice->enveShortAttackCount == 0))
  #endif
#else
	if ((pVoice->enveStep >= attackTabLen) ||
		(pVoice->enveShortAttackCount == 0))
#endif
		return enveEmuStartDecay(pVoice);
	else
	{
#if defined(DIRECT_FIXPOINT) && !defined(SID_FPUENVE)
  #if defined(SID_LINENVE)
		pVoice->enveVol = (ubyte)pVoice->enveStep.w[HI];
  #else
		pVoice->enveVol = attackTab[pVoice->enveStep.w[HI]];
  #endif
#else
  #if defined(SID_LINENVE)
		pVoice->enveVol = pVoice->enveStep;
  #else
		pVoice->enveVol = attackTab[pVoice->enveStep];
  #endif
#endif
	    pVoice->enveShortAttackCount--;
		enveEmuEnveAdvance(pVoice);
		return masterAmplModTable[ masterVolumeAmplIndex + pVoice->enveVol ];
	}
}

inline uword Envelope::enveEmuAlterShortAttack(struct sidOperator* pVoice) const
{
	ubyte attack = pVoice->SIDAD >> 4;
#ifdef SID_FPUENVE
	pVoice->fenveStepAdd = attackRates[attack];
#elif defined(DIRECT_FIXPOINT)
	pVoice->enveStepAdd.l = attackRates[attack];
#else
	pVoice->enveStepAdd = attackRates[attack];
	pVoice->enveStepAddPnt = attackRatesP[attack];
#endif
	pVoice->ADSRproc = &Envelope::enveEmuShortAttack;
	return enveEmuShortAttack(pVoice);
}

inline uword Envelope::enveEmuStartShortAttack(struct sidOperator* pVoice) const
{
	pVoice->ADSRctrl = ENVE_SHORTATTACK;
#ifdef SID_FPUENVE
  #if defined(SID_LINENVE)
	pVoice->fenveStep = (float)pVoice->enveVol;
  #else
	pVoice->fenveStep = attackPos[pVoice->enveVol];
  #endif
#elif defined(DIRECT_FIXPOINT)
  #if defined(SID_LINENVE)
	pVoice->enveStep.w[HI] = pVoice->enveVol;
  #else
	pVoice->enveStep.w[HI] = attackPos[pVoice->enveVol];
  #endif
	pVoice->enveStep.w[LO] = 0;
#else
  #if defined(SID_LINENVE)
	pVoice->enveStep = pVoice->enveVol;
  #else
	pVoice->enveStep = attackPos[pVoice->enveVol];
  #endif
	pVoice->enveStepPnt = 0;
#endif
	pVoice->enveShortAttackCount = 65535;  // unused
	return enveEmuAlterShortAttack(pVoice);
}



// EOF - ENVELOPE.CPP
