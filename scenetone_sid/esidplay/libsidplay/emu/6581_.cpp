//
// /home/ms/sidplay/libsidplay/emu/RCS/6581_.cpp,v
//
// Contributions:
//
// Noise generation algorithm is used courtesy of Asger Alstrup Nielsen.
// His original publication can be found on the SID home page.
//
// Noise table optimization proposed by Phillip Wooller. The output of
// each table does not differ.
//
// MOS-8580 R5 combined waveforms recorded by Dennis "Deadman" Lindroos.
// --------------------------------------------------------------------------
//
// --- MOS-6581 Emulator ---
//
// Copyright (c) 1994-1997 Michael Schwendt. All rights reserved.
//
// Redistribution and use  in source and  binary forms, either  unchanged or
// modified, are permitted provided that the following conditions are met:
//
// (1)  Redistributions  of  source  code  must  retain  the above copyright
// notice, this list of conditions and the following disclaimer.
//
// (2) Redistributions  in binary  form must  reproduce the  above copyright
// notice,  this  list  of  conditions  and  the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE  IS PROVIDED  BY THE  AUTHOR ``AS  IS'' AND  ANY EXPRESS OR
// IMPLIED  WARRANTIES,  INCLUDING,   BUT  NOT  LIMITED   TO,  THE   IMPLIED
// WARRANTIES OF MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR PURPOSE  ARE
// DISCLAIMED.  IN NO EVENT SHALL  THE AUTHOR OR CONTRIBUTORS BE LIABLE  FOR
// ANY DIRECT,  INDIRECT, INCIDENTAL,  SPECIAL, EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS
// OR SERVICES;  LOSS OF  USE, DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)
// HOWEVER  CAUSED  AND  ON  ANY  THEORY  OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN
// ANY  WAY  OUT  OF  THE  USE  OF  THIS  SOFTWARE,  EVEN  IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------


#if defined(SID_MONITOR) || defined(WB_DEBUG)
  #include <iostream.h>
  #include <iomanip.h>
#endif

#include "myendian.h"
#include "sidtune.h"
#include "6510_.h"
#include "emucfg.h"
#include "envelope.h"
#include "samples.h"
#include "wave6581.h"
#include "wave8580.h"
#include "6581_.h"
#include "mixing.h"


// constants
static const uword apSpeed = 0x4000;
static const udword noiseSeed = 0x7ffff8;

#define lowPassParam filterTable


// -------------------------------------------------------------------- Speed



inline void sidEmu::calcValuesPerCall()
{
	udword fastForwardFreq = PCMfreq;
	if ( fastForwardFactor != 128 )
	{
		fastForwardFreq = (PCMfreq * fastForwardFactor) >> 7;  // divide by 128
	}
#if defined(DIRECT_FIXPOINT)
   	VALUES.l = ( VALUESorg.l = (((fastForwardFreq<<12)/calls)<<4) );
	VALUESadd.l = 0;
#else
	VALUES = (VALUESorg = (fastForwardFreq / calls));
	VALUEScomma = ((fastForwardFreq % calls) * 65536UL) / calls;
	VALUESadd = 0;
#endif
}


void sidEmu::sidEmuChangeReplayingSpeed()
{
	calcValuesPerCall();
}

// PAL: Clock speed: 985248.4 Hz
//      CIA 1 Timer A: $4025 (60 Hz)
//
// NTSC: Clock speed: 1022727.14 Hz
//      CIA 1 Timer A: $4295 (60 Hz) 

void sidEmu::sidEmuSetClockSpeed(int clockMode)
{
	switch (clockMode)
	{ 
	 case SIDTUNE_CLOCK_NTSC:
		{
			C64_clockSpeed  = 1022727;
			C64_fClockSpeed = 1022727.14f;
			break;
		}
	 case SIDTUNE_CLOCK_PAL:
	 default:
		{
			C64_clockSpeed  = 985248;
			C64_fClockSpeed = (float)985248.4;
			break;
		}
	}
}


void sidEmu::sidEmuSetReplayingSpeed(int clockMode, uword callsPerSec)
{
	switch (clockMode)
	{ 
	 case SIDTUNE_CLOCK_NTSC:
		{
			sidtuneClockSpeed = 1022727;
			timer = (defaultTimer = 0x4295);
			break;
		}
	 case SIDTUNE_CLOCK_PAL:
	 default:
		{
			sidtuneClockSpeed = 985248;
			timer = (defaultTimer = 0x4025);
			break;
		}
	}
	switch (callsPerSec)
	{
	 case SIDTUNE_SPEED_CIA_1A:
		{
			timer = readLEword(c64mem2+0xdc04);
			if (timer < 16)  // prevent overflow
			{
				timer = defaultTimer;
			}
			calls = (uword)(sidtuneClockSpeed / timer);
			break;
		}
	 default:
		{
			calls = callsPerSec;
			break;
		}
	}
	calcValuesPerCall();
}
					

void sidEmu::sidEmuUpdateReplayingSpeed()
{ 
	if ( timer != readLEword(c64mem2+0xdc04) )
	{
		timer = readLEword(c64mem2+0xdc04);
		// Prevent overflow
		if ( timer < 16 )
			timer = defaultTimer;
		calls = (uword)(sidtuneClockSpeed / timer);
		calcValuesPerCall();
	}
}

// --------------------------------------------------------------------------

inline void waveAdvance(struct sidOperator* pVoice)
{
#if defined(DIRECT_FIXPOINT)
	pVoice->waveStep.l += pVoice->waveStepAdd.l;
	pVoice->waveStep.w[HI] &= 4095;
#else
	pVoice->waveStepPnt += pVoice->waveStepAddPnt;
	pVoice->waveStep += ( pVoice->waveStepAdd + ( pVoice->waveStepPnt > 65535 ));
	pVoice->waveStepPnt &= 0xFFFF;
	pVoice->waveStep &= 4095;
#endif
}

inline void sidEmu::noiseAdvance(struct sidOperator* pVoice)
{
	pVoice->noiseStep += pVoice->noiseStepAdd;
	if (pVoice->noiseStep >= (1L<<20))
	{
		pVoice->noiseStep -= (1L<<20);
#if defined(DIRECT_FIXPOINT)
		pVoice->noiseReg.l = (pVoice->noiseReg.l << 1) | 
			(((pVoice->noiseReg.l >> 22) ^ (pVoice->noiseReg.l >> 17)) & 1);
#else
		pVoice->noiseReg = (pVoice->noiseReg << 1) | 
			(((pVoice->noiseReg >> 22) ^ (pVoice->noiseReg >> 17)) & 1);
#endif
#if defined(DIRECT_FIXPOINT) && defined(LARGE_NOISE_TABLE)
		pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg.w[LO]]
							   |noiseTableMSB[pVoice->noiseReg.w[HI]&0xff]);
#elif defined(DIRECT_FIXPOINT)
		pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg.b[LOLO]]
							   |noiseTableMID[pVoice->noiseReg.b[LOHI]]
							   |noiseTableMSB[pVoice->noiseReg.b[HILO]]);
#else
		pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg&0xff]
							   |noiseTableMID[pVoice->noiseReg>>8&0xff]
							   |noiseTableMSB[pVoice->noiseReg>>16&0xff]);
#endif
	}
}

inline void sidEmu::noiseAdvanceHp(struct sidOperator* pVoice)
{
	udword tmp = pVoice->noiseStepAdd;
	while (tmp >= (1L<<20))
	{
		tmp -= (1L<<20);
#if defined(DIRECT_FIXPOINT)
		pVoice->noiseReg.l = (pVoice->noiseReg.l << 1) | 
			(((pVoice->noiseReg.l >> 22) ^ (pVoice->noiseReg.l >> 17)) & 1);
#else
		pVoice->noiseReg = (pVoice->noiseReg << 1) | 
			(((pVoice->noiseReg >> 22) ^ (pVoice->noiseReg >> 17)) & 1);
#endif
	}
	pVoice->noiseStep += tmp;
	if (pVoice->noiseStep >= (1L<<20))
	{
		pVoice->noiseStep -= (1L<<20);
#if defined(DIRECT_FIXPOINT)
		pVoice->noiseReg.l = (pVoice->noiseReg.l << 1) | 
			(((pVoice->noiseReg.l >> 22) ^ (pVoice->noiseReg.l >> 17)) & 1);
#else
		pVoice->noiseReg = (pVoice->noiseReg << 1) | 
			(((pVoice->noiseReg >> 22) ^ (pVoice->noiseReg >> 17)) & 1);
#endif
	}
#if defined(DIRECT_FIXPOINT) && defined(LARGE_NOISE_TABLE)
	pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg.w[LO]]
						   |noiseTableMSB[pVoice->noiseReg.w[HI]&0xff]);
#elif defined(DIRECT_FIXPOINT)
	pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg.b[LOLO]]
						   |noiseTableMID[pVoice->noiseReg.b[LOHI]]
						   |noiseTableMSB[pVoice->noiseReg.b[HILO]]);
#else
	pVoice->noiseOutput = (noiseTableLSB[pVoice->noiseReg&0xff]
						   |noiseTableMID[pVoice->noiseReg>>8&0xff]
						   |noiseTableMSB[pVoice->noiseReg>>16&0xff]);
#endif
}


#if defined(DIRECT_FIXPOINT)
  #define triangle triangleTable[pVoice->waveStep.w[HI]]
  #define sawtooth sawtoothTable[pVoice->waveStep.w[HI]]
  #define square squareTable[pVoice->waveStep.w[HI] + pVoice->pulseIndex]
  #define triSaw waveform30_8580[pVoice->waveStep.w[HI]]

#ifdef LESS_CODE_BUT_ABIT_SLOWER // use index 0 -> content 0 if index >= 4096
  #define triSquare (waveform50[( ((pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) : 0 )]) 
  #define sawSquare (waveform60_8580[( ((pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) : 0 )]) 
  #define triSawSquare (waveform70_8580[( ((pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth) : 0 )]) 
#else
  #define triSquare waveform50[pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth]
  #define sawSquare waveform60_8580[pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth]
  #define triSawSquare waveform70_8580[pVoice->waveStep.w[HI] + pVoice->SIDpulseWidth]
#endif

#else
  #define triangle triangleTable[pVoice->waveStep]
  #define sawtooth sawtoothTable[pVoice->waveStep]
  #define square squareTable[pVoice->waveStep + pVoice->pulseIndex]
  #define triSaw waveform30_8580[pVoice->waveStep]
#ifdef LESS_CODE_BUT_ABIT_SLOWER
  #define triSquare (waveform50[ ((pVoice->waveStep + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep + pVoice->SIDpulseWidth) : 0 ] )
  #define sawSquare (waveform60_8580[ ((pVoice->waveStep + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep + pVoice->SIDpulseWidth) : 0 ] )
  #define triSawSquare (waveform70_8580[ ((pVoice->waveStep + pVoice->SIDpulseWidth) < 4096) ? (pVoice->waveStep + pVoice->SIDpulseWidth) : 0 ] )
#else
  #define sawSquare waveform60_8580[pVoice->waveStep + pVoice->SIDpulseWidth]
  #define triSawSquare waveform70_8580[pVoice->waveStep + pVoice->SIDpulseWidth]
#endif

#endif


void sidEmu::sidMode00(struct sidOperator* pVoice)  {
	pVoice->output = (pVoice->filtIO-0x80);
	waveAdvance(pVoice);
}

/* TODO 
static void sidModeReal00(struct sidOperator* pVoice)  {
	pVoice->output = 0;
	waveAdvance(pVoice);
}
*/

void sidEmu::sidMode10(struct sidOperator* pVoice)  {
  pVoice->output = triangle;
  waveAdvance(pVoice);
}

void sidEmu::sidMode20(struct sidOperator* pVoice)  {
  pVoice->output = sawtooth;
  waveAdvance(pVoice);
}

void sidEmu::sidMode30(struct sidOperator* pVoice)  {
#ifdef SIDEMU_OLD_WAVE30
  pVoice->output = ( triangle & sawtooth );
#else
  pVoice->output = triSaw;
#endif
  waveAdvance(pVoice);
}

void sidEmu::sidMode40(struct sidOperator* pVoice)  {
  pVoice->output = square;
  waveAdvance(pVoice);
}

void sidEmu::sidMode50(struct sidOperator* pVoice)  {
  pVoice->output = triSquare;
  waveAdvance(pVoice);
}

void sidEmu::sidMode60(struct sidOperator* pVoice)  {
  pVoice->output = sawSquare;
  waveAdvance(pVoice);
}

void sidEmu::sidMode70(struct sidOperator* pVoice)  {
  pVoice->output = triSawSquare;
  waveAdvance(pVoice);
}

void sidEmu::sidMode80(struct sidOperator* pVoice)  {
  pVoice->output = pVoice->noiseOutput;
  waveAdvance(pVoice);
  noiseAdvance(pVoice);
}

void sidEmu::sidMode80hp(struct sidOperator* pVoice)  {
  pVoice->output = pVoice->noiseOutput;
  waveAdvance(pVoice);
  noiseAdvanceHp(pVoice);
}

void sidEmu::sidModeLock(struct sidOperator* pVoice)
{
	pVoice->noiseIsLocked = true;
	pVoice->output = (pVoice->filtIO-0x80);
	waveAdvance(pVoice);
}

//
// 
//

void sidEmu::sidMode14(struct sidOperator* pVoice)
{
#if defined(DIRECT_FIXPOINT)
  if ( pVoice->modulator->waveStep.w[HI] < 2048 )
#else
  if ( pVoice->modulator->waveStep < 2048 )
#endif
	pVoice->output = triangle;
  else
	pVoice->output = 0xFF ^ triangle;
  waveAdvance(pVoice);
}

void sidEmu::sidMode34(struct sidOperator* pVoice)  {
#if defined(DIRECT_FIXPOINT)
  if ( pVoice->modulator->waveStep.w[HI] < 2048 )
#else
  if ( pVoice->modulator->waveStep < 2048 )
#endif
#ifdef SIDEMU_OLD_WAVE30
	pVoice->output = triangle & sawtooth;
  else
	pVoice->output = (0xFF^triangle) & (0xFF^sawtooth);
#else
	pVoice->output = triSaw;
  else
	pVoice->output = 0xFF ^ triSaw;
#endif
  waveAdvance(pVoice);
}

void sidEmu::sidMode54(struct sidOperator* pVoice)  {
#if defined(DIRECT_FIXPOINT)
  if ( pVoice->modulator->waveStep.w[HI] < 2048 )
#else
  if ( pVoice->modulator->waveStep < 2048 )
#endif
	pVoice->output = triSquare;
  else
    pVoice->output = 0xFF ^ triSquare;
  waveAdvance(pVoice);
}

void sidEmu::sidMode74(struct sidOperator* pVoice)  {
#if defined(DIRECT_FIXPOINT)
  if ( pVoice->modulator->waveStep.w[HI] < 2048 )
#else
  if ( pVoice->modulator->waveStep < 2048 )
#endif
	pVoice->output = triSawSquare;
  else
    pVoice->output = 0xFF ^ triSawSquare;
  waveAdvance(pVoice);
}

//
// 
//

inline void waveCalcCycleLen(struct sidOperator* pVoice)
{
#if defined(DIRECT_FIXPOINT)
	pVoice->cycleAddLen.w[HI] = 0;
	pVoice->cycleAddLen.l += pVoice->cycleLen.l;
	pVoice->cycleLenCount = pVoice->cycleAddLen.w[HI];
#else		
	pVoice->cycleAddLenPnt += pVoice->cycleLenPnt;
	pVoice->cycleLenCount = pVoice->cycleLen + ( pVoice->cycleAddLenPnt > 65535 );
	pVoice->cycleAddLenPnt &= 0xFFFF;
#endif
	// If we keep the value cycleLen between 1 <= x <= 65535, 
	// the following check is not required.
//	if ( pVoice->cycleLenCount == 0 )
//	{
//#if defined(DIRECT_FIXPOINT)
//		pVoice->waveStep.l = 0;
//#else
//		pVoice->waveStep = (pVoice->waveStepPnt = 0);
//#endif
//		pVoice->cycleLenCount = 0;
//	}
//	else
//	{
#if defined(DIRECT_FIXPOINT)
		register uword diff = pVoice->cycleLenCount - pVoice->cycleLen.w[HI];
#else
		register uword diff = pVoice->cycleLenCount - pVoice->cycleLen;
#endif
		if ( pVoice->wavePre[diff].len != pVoice->cycleLenCount )
		{
			pVoice->wavePre[diff].len = (uword)(pVoice->cycleLenCount);
#if defined(DIRECT_FIXPOINT)
			pVoice->wavePre[diff].stp = (pVoice->waveStepAdd.l = (4096UL*65536UL) / pVoice->cycleLenCount);
#else
			pVoice->wavePre[diff].stp = (uword)(pVoice->waveStepAdd = 4096UL / pVoice->cycleLenCount);
			pVoice->wavePre[diff].pnt = (pVoice->waveStepAddPnt = ((4096UL % pVoice->cycleLenCount) * 65536UL) / pVoice->cycleLenCount);
#endif
		}
		else
		{
#if defined(DIRECT_FIXPOINT)
			pVoice->waveStepAdd.l = pVoice->wavePre[diff].stp;
#else
			pVoice->waveStepAdd = pVoice->wavePre[diff].stp;
			pVoice->waveStepAddPnt = pVoice->wavePre[diff].pnt;
#endif
		}
//	}  // see above (opening bracket)
}

inline void sidEmu::waveCalcFilter(struct sidOperator* pVoice)
{
	if ( pVoice->filtEnabled )
	{
		if ( filterType != 0 )
		{
			if ( filterType == 0x20 )
			{
				pVoice->filtLow += ( pVoice->filtRef * filterDy );
				float tmp = (float)pVoice->filtIO - pVoice->filtLow;
				tmp -= pVoice->filtRef * filterResDy;
				pVoice->filtRef += ( tmp * (filterDy) );
				pVoice->filtIO = (sbyte)(pVoice->filtRef-pVoice->filtLow/4);
			}
			else if (filterType == 0x40)
			{
				pVoice->filtLow += (float)( pVoice->filtRef * filterDy * 0.1 );
				float tmp = (float)pVoice->filtIO - pVoice->filtLow;
				tmp -= pVoice->filtRef * filterResDy;
				pVoice->filtRef += ( tmp * (filterDy) );
				float tmp2 = pVoice->filtRef - pVoice->filtIO/8;
				if (tmp2 < -128)
					tmp2 = -128;
				if (tmp2 > 127)
					tmp2 = 127;
				pVoice->filtIO = (sbyte)tmp2;
			}
			else
			{
				pVoice->filtLow += ( pVoice->filtRef * filterDy );
				float sample = pVoice->filtIO;
				float sample2 = sample - pVoice->filtLow;
				int tmp = (int)sample2;
				sample2 -= pVoice->filtRef * filterResDy;
				pVoice->filtRef += ( sample2 * filterDy );
			
				if ( filterType == 0x10 )
				{
					pVoice->filtIO = (sbyte)pVoice->filtLow;
				}
				else if ( filterType == 0x30 )
				{
					pVoice->filtIO = (sbyte)pVoice->filtLow;
				}
				else if ( filterType == 0x50 )
				{
					pVoice->filtIO = (sbyte)(sample - (tmp >> 1));
				}
				else if ( filterType == 0x60 )
				{
					pVoice->filtIO = (sbyte)tmp;
				}
				else if ( filterType == 0x70 )
				{
					pVoice->filtIO = (sbyte)(sample - (tmp >> 1));
				}
			}
		}
		else // filterType == 0x00
		{
			pVoice->filtIO = 0;
		}
	}
}


sbyte sidEmu::waveCalcMute(struct sidOperator* pVoice)
	{
//	iTheEnvelope->callADSRproc(pVoice);
	
	(iTheEnvelope->*pVoice->ADSRproc)(pVoice);  // just process envelope
	return pVoice->filtIO;
	}


sbyte sidEmu::waveCalcNormal(struct sidOperator* pVoice)
	{
 
	if ( pVoice->cycleLenCount <= 0 )
	{
		waveCalcCycleLen(pVoice);
		if (( pVoice->SIDctrl & 0x40 ) == 0x40 )
		{
			pVoice->pulseIndex = pVoice->newPulseIndex;
			if ( pVoice->pulseIndex > 2048 )
			{
#if defined(DIRECT_FIXPOINT)
				pVoice->waveStep.w[HI] = 0;
#else
				pVoice->waveStep = 0;
#endif
			}
		}
	}

	(this->*pVoice->waveProc)(pVoice);
//	(*pVoice->waveProc)(pVoice); - original

//	int index = iTheEnvelope->callADSRproc(pVoice);
	int index = (iTheEnvelope->*pVoice->ADSRproc)(pVoice);

	pVoice->filtIO = iTheEmuEngine->ampMod1x8[index | pVoice->output];
//	pVoice->filtIO = iTheEmuEngine->ampMod1x8[(*pVoice->ADSRproc)(pVoice)|pVoice->output];
	((sidEmu*)this)->waveCalcFilter(pVoice); //TODO: check this

	return pVoice->filtIO;
}

sbyte sidEmu::waveCalcRangeCheck(struct sidOperator* pVoice)
	{

#if defined(DIRECT_FIXPOINT)
	pVoice->waveStepOld = pVoice->waveStep.w[HI];
	(this->*pVoice->waveProc)(pVoice);
	if (pVoice->waveStep.w[HI] < pVoice->waveStepOld)
#else
	pVoice->waveStepOld = pVoice->waveStep;
	(this->*pVoice->waveProc)(pVoice);
	if (pVoice->waveStep < pVoice->waveStepOld)
#endif
	{
		// Next step switch back to normal calculation.
		pVoice->cycleLenCount = 0;
		pVoice->outProc = &sidEmu::waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
				pVoice->waveStep.w[HI] = 4095;
#else
				pVoice->waveStep = 4095;
#endif
	}

	int index = (iTheEnvelope->*pVoice->ADSRproc)(pVoice);
	pVoice->filtIO = iTheEmuEngine->ampMod1x8[index | pVoice->output];

//	pVoice->filtIO = iTheEmuEngine->ampMod1x8[iTheEnvelope->callADSRproc(pVoice)|pVoice->output];
//	pVoice->filtIO = iTheEmuEngine->ampMod1x8[(*pVoice->ADSRproc)(pVoice)|pVoice->output];
	((sidEmu*)this)->waveCalcFilter(pVoice); //TODO: check this
	return pVoice->filtIO;
}

// -------------------------------------------------- Operator frame set-up 1

inline void sidEmu::sidEmuSet(struct sidOperator* pVoice, uword sidIndex)
{
	pVoice->SIDfreq = readLEword(c64mem2+sidIndex);

	pVoice->SIDpulseWidth = (readLEword(c64mem2+sidIndex+2) & 0x0FFF);
	pVoice->newPulseIndex = 4096 - pVoice->SIDpulseWidth;
#if defined(DIRECT_FIXPOINT)
	if ( ((pVoice->waveStep.w[HI] + pVoice->pulseIndex) >= 0x1000)
		&& ((pVoice->waveStep.w[HI] + pVoice->newPulseIndex) >= 0x1000) )
	{
		pVoice->pulseIndex = pVoice->newPulseIndex;
	}
	else if ( ((pVoice->waveStep.w[HI] + pVoice->pulseIndex) < 0x1000)
		&& ((pVoice->waveStep.w[HI] + pVoice->newPulseIndex) < 0x1000) )
	{
		pVoice->pulseIndex = pVoice->newPulseIndex;
	}
#else
	if ( ((pVoice->waveStep + pVoice->pulseIndex) >= 0x1000)
		&& ((pVoice->waveStep + pVoice->newPulseIndex) >= 0x1000) )
	{
		pVoice->pulseIndex = pVoice->newPulseIndex;
	}
	else if ( ((pVoice->waveStep + pVoice->pulseIndex) < 0x1000)
		&& ((pVoice->waveStep + pVoice->newPulseIndex) < 0x1000) )
	{
		pVoice->pulseIndex = pVoice->newPulseIndex;
	}
#endif

    ubyte enveTemp, newWave, oldWave;
	
	oldWave = pVoice->SIDctrl;
	enveTemp = pVoice->ADSRctrl;
	pVoice->SIDctrl = (newWave = c64mem2[sidIndex +4]);

	if (( newWave & 1 ) ==0 )
	{
		if (( oldWave & 1 ) !=0 )
			enveTemp = ENVE_STARTRELEASE;
//		else if ( pVoice->gateOnCtrl )
//		{
//			enveTemp = ENVE_STARTSHORTATTACK;
//		}
	}
	else if ( pVoice->gateOffCtrl || ((oldWave&1)==0) )
	{
		enveTemp = ENVE_STARTATTACK;
		if (doAutoPanning && updateAutoPanning)
		{
			// Swap source/destination position.
			uword tmp = pVoice->gainSource;
			pVoice->gainSource = pVoice->gainDest;
			pVoice->gainDest = tmp;
			if ((pVoice->gainDest^pVoice->gainSource) == 0)
			{
				// Mute voice.
				pVoice->gainLeft = (pVoice->gainRight = 0x0000+0x80);
			}
			else
			{
				// Start from middle position.
				pVoice->gainLeft = pVoice->gainLeftCentered;
				pVoice->gainRight = pVoice->gainRightCentered;
			}
			// Determine direction.
			// true  = L > R : L down, R up
			// false = L < R : L up, R down
			pVoice->gainDirec = (pVoice->gainLeft > pVoice->gainDest);
		}
	}

	if (doAutoPanning && updateAutoPanning && (enveTemp!=ENVE_STARTATTACK))
	{
		if (pVoice->gainDirec)
		{
			if (pVoice->gainLeft > pVoice->gainDest)
			{
				pVoice->gainLeft -= 0x0100;
				pVoice->gainRight += 0x0100;
			}
			else
			{
				// Swap source/destination position.
				uword tmp = pVoice->gainSource;
				pVoice->gainSource = pVoice->gainDest;
				pVoice->gainDest = tmp;
				// Inverse direction.
				pVoice->gainDirec = false;
			}
		}
		else
		{
			if (pVoice->gainRight > pVoice->gainSource)
			{
				pVoice->gainLeft += 0x0100;
				pVoice->gainRight -= 0x0100;
			}
			else
			{
				pVoice->gainDirec = true;
				// Swap source/destination position.
				uword tmp = pVoice->gainSource;
				pVoice->gainSource = pVoice->gainDest;
				// Inverse direction.
				pVoice->gainDest = tmp;
			}
		}
	}

	if ((( oldWave ^ newWave ) & 0xF0 ) != 0 )
	{
		pVoice->cycleLenCount = 0;
	}
	
	ubyte ADtemp = c64mem2[sidIndex +5];
	ubyte SRtemp = c64mem2[sidIndex +6];
	if ( pVoice->SIDAD != ADtemp )
	{
		enveTemp |= ENVE_ALTER;
	}
	else if ( pVoice->SIDSR != SRtemp )
	{
		enveTemp |= ENVE_ALTER;
	}
	pVoice->SIDAD = ADtemp;
	pVoice->SIDSR = SRtemp;
	extern const ubyte masterVolumeLevels[16];  // -> envelope.cpp
	ubyte tmpSusVol = masterVolumeLevels[SRtemp >> 4];
	if (pVoice->ADSRctrl != ENVE_SUSTAIN)  // !!!
	{
		pVoice->enveSusVol = tmpSusVol;
	}
	else
	{
		if ( pVoice->enveSusVol > pVoice->enveVol )
			pVoice->enveSusVol = 0;
		else
			pVoice->enveSusVol = tmpSusVol;
	}
	
	pVoice->ADSRproc = (iTheEnvelope->getEnveModeTable())[enveTemp>>1];  // shifting out the KEY-bit
	pVoice->ADSRctrl = enveTemp & (255-ENVE_ALTER-1);
  
	if ( filterEnabled )
	{
		if (( c64mem2[0xd417] & pVoice->filtVoiceMask ) != 0 )
		{
			pVoice->filtEnabled = true;
		}
		else
		{
			pVoice->filtEnabled = false;
		}
	}
	else
	{
		pVoice->filtEnabled = false;
	}
}

// -------------------------------------------------- Operator frame set-up 2
#if 0 //TODO
// MOS-8580, MOS-6581 (no 70)
ptr2sidVoidFunc sidEmu::sidModeNormalTable[16] =
{
  sidEmu::sidMode00, sidEmu::sidMode10, sidEmu::sidMode20, sidEmu::sidMode30, sidEmu::sidMode40, sidEmu::sidMode50, sidEmu::sidMode60, sidEmu::sidMode70,
  sidEmu::sidMode80, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock
};

// MOS-8580, MOS-6581 (no 74)
ptr2sidVoidFunc sidEmu::sidModeRingTable[16] =
{
  sidEmu::sidMode00, sidEmu::sidMode14, sidEmu::sidMode00, sidEmu::sidMode34, sidEmu::sidMode00, sidEmu::sidMode54, sidEmu::sidMode00, sidEmu::sidMode74,
  sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock, sidEmu::sidModeLock
};
#endif

inline void sidEmu::sidEmuSet2(struct sidOperator* pVoice)
{
	pVoice->outProc = &sidEmu::waveCalcNormal;
	pVoice->sync = false;

	if ( (pVoice->SIDfreq < 16)
		|| ((pVoice->SIDctrl & 8) != 0) )
	{
		pVoice->outProc = &sidEmu::waveCalcMute;
		if (pVoice->SIDfreq == 0)
		{
#if defined(DIRECT_FIXPOINT)
			pVoice->cycleLen.l = (pVoice->cycleAddLen.l = 0);
			pVoice->waveStep.l = 0;
#else
			pVoice->cycleLen = (pVoice->cycleLenPnt = 0);
			pVoice->cycleAddLenPnt = 0;
			pVoice->waveStep = 0;
			pVoice->waveStepPnt = 0;
#endif
			pVoice->curSIDfreq = (pVoice->curNoiseFreq = 0);
			pVoice->noiseStepAdd = 0;
			pVoice->cycleLenCount = 0;
		}
		if ((pVoice->SIDctrl & 8) != 0)
		{
			if (pVoice->noiseIsLocked)
			{
				pVoice->noiseIsLocked = false;
#if defined(DIRECT_FIXPOINT)
				pVoice->noiseReg.l = noiseSeed;
#else
				pVoice->noiseReg = noiseSeed;
#endif
			}
		}
	}
	else
	{
		if ( pVoice->curSIDfreq != pVoice->SIDfreq )
		{
			pVoice->curSIDfreq = (uword)(pVoice->SIDfreq);
			// We keep the value cycleLen between 1 <= x <= 65535.
			// This makes a range-check in waveCalcCycleLen() unrequired.
#if defined(DIRECT_FIXPOINT)
			pVoice->cycleLen.l = ((PCMsid << 12) / pVoice->SIDfreq) << 4;
			if (pVoice->cycleLenCount > 0)
			{
				waveCalcCycleLen(pVoice);
				pVoice->outProc = &sidEmu::waveCalcRangeCheck;
			}
#else
			pVoice->cycleLen = (uword)(PCMsid / pVoice->SIDfreq);
			pVoice->cycleLenPnt = (uword)((( PCMsid % pVoice->SIDfreq ) * 65536UL ) / pVoice->SIDfreq);
			if (pVoice->cycleLenCount > 0)
			{
				waveCalcCycleLen(pVoice);
				pVoice->outProc = &sidEmu::waveCalcRangeCheck;
			}
#endif
		}
  
		if ((( pVoice->SIDctrl & 0x80 ) == 0x80 ) && ( pVoice->curNoiseFreq != pVoice->SIDfreq ))
		{
			pVoice->curNoiseFreq = (uword)(pVoice->SIDfreq);
			pVoice->noiseStepAdd = (PCMsidNoise * pVoice->SIDfreq) >> 8;
			if (pVoice->noiseStepAdd >= (1L<<21))
				sidModeNormalTable[8] = &sidEmu::sidMode80hp;
			else
				sidModeNormalTable[8] = &sidEmu::sidMode80;
		}
	
		if (( pVoice->SIDctrl & 2 ) != 0 )
		{
			if ( ( pVoice->modulator->SIDfreq == 0 ) || (( pVoice->modulator->SIDctrl & 8 ) != 0 ) )
			{
				;
			}
			else if ( (( pVoice->carrier->SIDctrl & 2 ) != 0 ) &&
					 ( pVoice->modulator->SIDfreq >= ( pVoice->SIDfreq << 1 )) )
			{
				;
			}
			else
			{
				pVoice->sync = true;
			}
		}
		
		if ((( pVoice->SIDctrl & 0x14 ) == 0x14 ) && ( pVoice->modulator->SIDfreq != 0 ))
			pVoice->waveProc = sidModeRingTable[pVoice->SIDctrl >> 4];
		else
			pVoice->waveProc = sidModeNormalTable[pVoice->SIDctrl >> 4];
	}
}



void sidEmu::sidEmuFillBuffer( emuEngine& thisEmu,
					   sidTune& thistune, 
					   void* buffer, udword bufferLen )
	{

	

	// Ensure a sane status of the whole emulator.
	if ( thisEmu.isReady && thistune.returnStatus() )
		{
		// split sample buffer into pieces for # voices
		if ( thisEmu.config.volumeControl == SIDEMU_HWMIXING )
			{
			bufferLen >>= 2; // or /4
			splitBufferLen = bufferLen;
			}
		// both, 16-bit and stereo samples take more memory
		// hence fewer samples fit into the buffer
		bufferLen >>= thisEmu.bufferScale;
		
#if defined(SIDEMU_TIME_COUNT)
		if (prevBufferLen != bufferLen)
			{
			prevBufferLen = bufferLen;
			scaledBufferLen = (bufferLen<<7) / fastForwardFactor;
			}
		thisEmu.bytesCount += scaledBufferLen;
		while (thisEmu.bytesCount >= thisEmu.config.frequency)
			{
			thisEmu.bytesCount -= thisEmu.config.frequency;
			thisEmu.secondsThisSong++;
			thisEmu.secondsTotal++;
			}
#endif
		
		while ( bufferLen > 0 )
			{
			if ( toFill > bufferLen )
				{
				buffer = thisEmu.iTheMixer->fillFunc(buffer, bufferLen); //AEH
				toFill -= (uword)bufferLen;
				bufferLen = 0;
				}
			else if ( toFill > 0 )
				{
				buffer = thisEmu.iTheMixer->fillFunc(buffer, toFill); //AEH - HERE IS THE BUG!!!
				bufferLen -= toFill;
				toFill = 0;
				}
			
			if ( toFill == 0 )
				{
				iThe6510->optr3readWave = optr3.output;
				iThe6510->optr3readEnve = optr3.enveVol;
				
				uword replayPC = thistune.returnPlayAddr();
				// playRamRom was set by external player interface.
				if ( replayPC == 0 )
					{
 					thisEmu.playRamRom = c64mem1[1];
 					if ((thisEmu.playRamRom & 2) != 0)  // isKernal ?
 						replayPC = readLEword(c64mem1+0x0314);  // IRQ
 					else
 						replayPC = readLEword(c64mem1+0xfffe);  // NMI
					}
				//bool retcode = 
				iThe6510->interpreter(replayPC, thisEmu.playRamRom, 0, 0, 0);
				
				if (thistune.returnSongSpeed() == 0)
					{
					sidEmuUpdateReplayingSpeed();
					}
				
				iTheEnvelope->masterVolume = ( c64mem2[0xd418] & 15 );
				iTheEnvelope->masterVolumeAmplIndex = iTheEnvelope->masterVolume << 8;

				optr1.gateOnCtrl = iThe6510->sidKeysOn[4];
				optr1.gateOffCtrl = iThe6510->sidKeysOff[4];
				sidEmuSet( &optr1, 0xd400 );
				optr2.gateOnCtrl = iThe6510->sidKeysOn[4+7];
				optr2.gateOffCtrl = iThe6510->sidKeysOff[4+7];
				sidEmuSet( &optr2, 0xd407 );
				optr3.gateOnCtrl = iThe6510->sidKeysOn[4+14];
				optr3.gateOffCtrl = iThe6510->sidKeysOff[4+14];
				sidEmuSet( &optr3, 0xd40e );
				
				filterType = c64mem2[0xd418] & 0x70;
				if (filterType != filterCurType)
					{
					filterCurType = filterType;
					optr1.filtLow = (optr1.filtRef = 0);
					optr2.filtLow = (optr2.filtRef = 0);
					optr3.filtLow = (optr3.filtRef = 0);
					}
				if ( filterEnabled )
					{

					filterValue = 0x7ff & ( (c64mem2[0xd415]&7) | ( (uword)c64mem2[0xd416] << 3 ));
					if (filterType == 0x20)
						filterDy = bandPassParam[filterValue];
					else
						filterDy = lowPassParam[filterValue];
					filterResDy = filterResTable[c64mem2[0xd417] >> 4] - filterDy;
					if ( filterResDy < 1.0 )
						filterResDy = 1.0;
					}
				
				sidEmuSet2( &optr1 );
				sidEmuSet2( &optr2 );  // somewhere here...
				sidEmuSet2( &optr3 );
				
				iTheSampler->sampleEmuCheckForInit();
				
#if defined(DIRECT_FIXPOINT)
				VALUESadd.w[HI] = 0;
				VALUESadd.l += VALUES.l;
				toFill = VALUESadd.w[HI];
#else
				udword temp = (VALUESadd + VALUEScomma);
				VALUESadd = temp & 0xFFFF;
				toFill = VALUES + (temp > 65535);
#endif
				
				// Decide whether to update/start auto-panning.
				if ((apCount += timer) >= apSpeed)
					{
					apCount -= apSpeed;
					updateAutoPanning = true;
					}
				else
					updateAutoPanning = false;
				
				}
			} // end while bufferLen
		} // end if status
	}


bool sidEmu::sidEmuFastForwardReplay( int percent )
{
	if (( percent < 1 ) || ( percent > 100 ))
	{
		return false;
	}
	else
	{
		fastForwardFactor = (percent<<7)/100;  // we use 2^7 as divider
#if defined(SIDEMU_TIME_COUNT)
		scaledBufferLen = (prevBufferLen<<7)/fastForwardFactor;
#endif
		calcValuesPerCall();
		// Ensure that we calculate at least a single sample per player call.
		// Still possible would be also (0 < x < 1.0).
		// Else (x = 0) this would cause a deadlock in the buffer fill loop.
#if defined(DIRECT_FIXPOINT)
		if (VALUES.w[HI] < 1)
		{
			VALUES.l = (VALUESorg.l = 0);
			VALUES.w[HI] = (VALUESorg.w[HI] = 1);
		}
#else
		if (VALUES < 1)
		{
			VALUES = (VALUESorg = 1);
			VALUEScomma = 0;
		}
#endif
		return true;
	}
}

// --------------------------------------------------------------------- Init

void sidEmu::initWaveformTables(bool isNewSID)
{
	int i,j;
	uword k;
	
	k = 0;
	for ( i = 0; i < 256; i++ )
		for ( j = 0; j < 8; j++ )
			triangleTable[k++] = (ubyte)i;
	for ( i = 255; i >= 0; i-- )
		for ( j = 0; j < 8; j++ )
			triangleTable[k++] = (ubyte)i;

//////
//	in    | out
// 0x0000 | 0x00
// 0x0001 | 0x00
// 0x0008 | 0x01
// 0x0fff | 0xff
// 0x1000 | 0xff
// 0x1fff | 0x00



	k = 0;
	for ( i = 0; i < 256; i++ )
		for ( j = 0; j < 16; j++ )
			sawtoothTable[k++] = (ubyte)i;

// 00000000000000001111111111111111222222222
//
//  out = sawtooth[in]
//
//  in    | out
//  0x00  | 0x00
//  0x01  | 0x00
//  0x10  | 0x01
//  0xfff | 0xff
//  
// -> out = in >> 4; TODO for SawTooth table
 	
	k = 0;
	for ( i = 0; i < 4096; i++ )  
		squareTable[k++] = 0;
	for ( i = 0; i < 4096; i++ )  
		squareTable[k++] = 255;

// in     | out
// 0x0000 | 0x00
// 0x0fff | 0x00
// 0x1000 | 0xff
// 0x1fff | 0xff
//
// -> out = in & 0x1000 ? 255 : 0;


#ifdef SIDEMU_OLD_WAVE50
	ubyte makeSIDsData1[17*8] =
	{
		0x00,0x00,0x00,0x00,0x00,0x40,0x60,0x7f,		// 0x78
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		// 0x80
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		// 0x88
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		// 0x90
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,		// 0x98
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		// 0xa0
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,		// 0xa8
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,		// 0xb0
		0x00,0x00,0x00,0x80,0x00,0x80,0x80,0xbf,		// 0xb8
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		// 0xc0
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,		// 0xc8
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,		// 0xd0
		0x00,0x00,0x00,0xc0,0x00,0xc0,0xc0,0xdf,		// 0xd8
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,		// 0xe0
		0x00,0x00,0x00,0xc0,0xe0,0xe0,0xe0,0xef,		// 0xe8
		0x00,0x00,0x80,0xe0,0x80,0xe0,0xe0,0xf7,		// 0xf0
		0x80,0xe0,0xf0,0xfb,0xf0,0xfd,0xfe,0xff         // 0xf8
	};
	k = 0;
	for ( i = 0; i < 4096; i++ )
	{
		ubyte tmp = triangleTable[i];
		if (tmp < 0x78)
		{
			tmp = 0;
		}
		else
		{
			tmp = makeSIDsData1[tmp-0x78];
		}
		waveform50[k++] = tmp;
	}
#else
	if ( isNewSID )
	{
		for ( i = 0; i < 4096; i++ )
			waveform50[i] = waveform50_8580[i];
	}
	else
	{
		int dest = 0;
		for ( i = 0; i < 512; i++ )
			for ( j = 0; j < 8; j++ )
				waveform50[dest++] = waveform50_6581[i];
	}
#endif

	for ( i = 4096; i < 8192; i++ )
	{
#ifndef LESS_CODE_BUT_ABIT_SLOWER
		waveform50[i] = 0;
		waveform60_8580[i] = 0;
		waveform70_8580[i] = 0;
#endif

	}
	
	if ( isNewSID )
	{
		sidModeNormalTable[3] = &sidEmu::sidMode30;
		sidModeNormalTable[6] = &sidEmu::sidMode60;
		sidModeNormalTable[7] = &sidEmu::sidMode70;
		sidModeRingTable[7] = &sidEmu::sidMode74;
	}
	else
	{
		sidModeNormalTable[3] = &sidEmu::sidMode30;
		sidModeNormalTable[6] = &sidEmu::sidMode60;
		sidModeNormalTable[7] = &sidEmu::sidMode00;
		sidModeRingTable[7] = &sidEmu::sidMode00;
	}

#if defined(LARGE_NOISE_TABLE)
	udword ni;
	for (ni = 0; ni < sizeof(noiseTableLSB); ni++)
	{
		noiseTableLSB[ni] = (ubyte)
			(((ni >> (13-4)) & 0x10) |
			 ((ni >> (11-3)) & 0x08) |
			 ((ni >> (7-2)) & 0x04) |
			 ((ni >> (4-1)) & 0x02) |
			 ((ni >> (2-0)) & 0x01));
	}
	for (ni = 0; ni < sizeof(noiseTableMSB); ni++)
	{
		noiseTableMSB[ni] = (ubyte)
			(((ni << (7-(22-16))) & 0x80) |
			 ((ni << (6-(20-16))) & 0x40) |
			 ((ni << (5-(16-16))) & 0x20));
	}
#else
	udword ni;
	for (ni = 0; ni < sizeof(noiseTableLSB); ni++)
	{
		noiseTableLSB[ni] = (ubyte)
			(((ni >> (7-2)) & 0x04) |
			 ((ni >> (4-1)) & 0x02) |
			 ((ni >> (2-0)) & 0x01));
	}
	for (ni = 0; ni < sizeof(noiseTableMID); ni++)
	{
		noiseTableMID[ni] = (ubyte)
			(((ni >> (13-8-4)) & 0x10) |
			 ((ni << (3-(11-8))) & 0x08));
	}
	for (ni = 0; ni < sizeof(noiseTableMSB); ni++)
	{
		noiseTableMSB[ni] = (ubyte)
			(((ni << (7-(22-16))) & 0x80) |
			 ((ni << (6-(20-16))) & 0x40) |
			 ((ni << (5-(16-16))) & 0x20));
	}
#endif
}


void sidEmu::sidEmuConfigure(const struct emuConfig& aEmuConfig)
{
	sidEmuSetClockSpeed(aEmuConfig.clockSpeed);  // set clock speed

	PCMfreq = aEmuConfig.frequency;
//	PCMsid = (udword)(PCMfrequency * (16777216.0 / C64_fClockSpeed)); // This is the original (working) __fixunsdfsi
//	PCMsidNoise = (udword)((C64_fClockSpeed*256.0)/PCMfrequency);
//	PCMsid = (sdword)(PCMfrequency * (16777216.0 / C64_fClockSpeed)); // this is also safe
//	PCMsidNoise = (sdword)((C64_fClockSpeed*256.0)/PCMfrequency);
	PCMsid = (udword)(aEmuConfig.frequency * (16777216 / C64_clockSpeed)); //TODO! use fClockSpeed instead ! 
	PCMsidNoise = (udword)((C64_clockSpeed*256) / aEmuConfig.frequency);

	sidEmuChangeReplayingSpeed();       // depends on frequency
	iTheSampler->sampleEmuInit(this);   // depends on clock speed + frequency
	
	filterEnabled = aEmuConfig.emulateFilter;
	initWaveformTables(aEmuConfig.mos8580);
	
	iTheEnvelope->enveEmuInit(PCMfreq, aEmuConfig.measuredVolume);
}


// Reset.

bool sidEmu::sidEmuReset()
{
	clearSidOperator( &optr1 );
	iTheEnvelope->enveEmuResetOperator( &optr1 );
	clearSidOperator( &optr2 );
	iTheEnvelope->enveEmuResetOperator( &optr2 );
	clearSidOperator( &optr3 );
	iTheEnvelope->enveEmuResetOperator( &optr3 );
	
	optr1.modulator = &optr3;
	optr3.carrier = &optr1;
	optr1.filtVoiceMask = 1;
	
	optr2.modulator = &optr1;
	optr1.carrier = &optr2;
	optr2.filtVoiceMask = 2;
	
	optr3.modulator = &optr2;
	optr2.carrier = &optr3;
	optr3.filtVoiceMask = 4;
	
	// Used for detecting changes of the GATE-bit (aka KEY-bit).
	// 6510-interpreter clears these before each call.
	iThe6510->sidKeysOff[4] = 
	  (iThe6510->sidKeysOff[4+7] = 
	   (iThe6510->sidKeysOff[4+14] = false));
	iThe6510->sidKeysOn[4] = 
	  (iThe6510->sidKeysOn[4+7] = 
	   (iThe6510->sidKeysOn[4+14] = false));
	
	iTheSampler->sampleEmuReset(this);
	
	filterType = (filterCurType = 0);
	filterValue = 0;
	filterDy = (filterResDy = 0);
	
	toFill = 0;
#if defined(SIDEMU_TIME_COUNT)
	prevBufferLen = (scaledBufferLen = 0);
#endif
	
	return true;
}


void sidEmu::clearSidOperator( struct sidOperator* pVoice )
{
	pVoice->SIDfreq = 0;
	pVoice->SIDctrl = 0;
	pVoice->SIDAD = 0;
	pVoice->SIDSR = 0;

	pVoice->sync = false;
	
	pVoice->pulseIndex = (pVoice->newPulseIndex = (pVoice->SIDpulseWidth = 0));
	pVoice->curSIDfreq = (pVoice->curNoiseFreq = 0);
	
	pVoice->output = (pVoice->noiseOutput = 0);
	pVoice->filtIO = 0;
	
	pVoice->filtEnabled = false;
	pVoice->filtLow = (pVoice->filtRef = 0);

	pVoice->cycleLenCount = 0;
#if defined(DIRECT_FIXPOINT)
	pVoice->cycleLen.l = (pVoice->cycleAddLen.l = 0);
#else
	pVoice->cycleLen = (pVoice->cycleLenPnt = 0);
	pVoice->cycleAddLenPnt = 0;
#endif
	
	pVoice->outProc = &sidEmu::waveCalcMute;

#if defined(DIRECT_FIXPOINT)
	pVoice->waveStepAdd.l = (pVoice->waveStep.l = 0);
	pVoice->wavePre[0].len = (uword)(pVoice->wavePre[0].stp = 0);
	pVoice->wavePre[1].len = (uword)(pVoice->wavePre[1].stp = 0);
#else
	pVoice->waveStep = 0;
	pVoice->waveStepAdd = 0;
	pVoice->waveStepPnt = 0;
	pVoice->waveStepAddPnt = 0;
	pVoice->wavePre[0].len = 0;
	pVoice->wavePre[0].stp = 0;
	pVoice->wavePre[0].pnt = 0;
	pVoice->wavePre[1].len = 0;
	pVoice->wavePre[1].stp = 0;
	pVoice->wavePre[1].pnt = 0;
#endif
	pVoice->waveStepOld = 0;

#if defined(DIRECT_FIXPOINT)
	pVoice->noiseReg.l = noiseSeed;
#else
	pVoice->noiseReg = noiseSeed;
#endif
	pVoice->noiseStepAdd = (pVoice->noiseStep = 0);
	pVoice->noiseIsLocked = false;
}


void sidEmu::sidEmuResetAutoPanning(int autoPanning)
{
	doAutoPanning = (autoPanning!=SIDEMU_NONE);
	updateAutoPanning = false;
	apCount = 0;
	// Auto-panning see sidEmuSet(). Reset volume levels to default.
	if (doAutoPanning)
	{
		optr1.gainLeft = (optr1.gainSource = 0xa080);
		optr1.gainRight = (optr1.gainDest = 0x2080);
		optr1.gainDirec = (optr1.gainLeft > optr1.gainRight);
		optr1.gainLeftCentered = 0x8080;  // middle
		optr1.gainRightCentered = 0x7f80;
		
		optr2.gainLeft = (optr2.gainSource = 0x2080);  // this one mirrored
		optr2.gainRight = (optr2.gainDest = 0xa080);
		optr2.gainDirec = (optr2.gainLeft > optr2.gainRight);
		optr2.gainLeftCentered = 0x8080;  // middle
		optr2.gainRightCentered = 0x7f80;
		
		optr3.gainLeft = (optr3.gainSource = 0xa080);
		optr3.gainRight = (optr3.gainDest = 0x2080);
		optr3.gainDirec = (optr3.gainLeft > optr3.gainRight);
		optr3.gainLeftCentered = 0x8080;  // middle
		optr3.gainRightCentered = 0x7f80;
		
		voice4_gainLeft = 0x8080;   // middle, not moving
		voice4_gainRight = 0x7f80;
	}
}


void sidEmu::sidEmuSetVoiceVolume(int voice, uword leftLevel, uword rightLevel, uword total)
{
	leftLevel *= total;
	leftLevel >>= 8;
	rightLevel *= total;
	rightLevel >>= 8;
	uword centeredLeftLevel = (0x80*total)>>8;
	uword centeredRightLevel = (0x7f*total)>>8;
	// Signed 8-bit samples will be added to base array index. 
	// So middle must be 0x80.
	// [-80,-81,...,-FE,-FF,0,1,...,7E,7F]
	uword leftIndex = 0x0080 + (leftLevel<<8);
	uword rightIndex = 0x0080 + (rightLevel<<8);
	uword gainLeftCentered = 0x0080 + (centeredLeftLevel<<8);
	uword gainRightCentered = 0x0080 + (centeredRightLevel<<8);
	switch ( voice )
	{
	 case 1:
		{
			optr1.gainLeft = leftIndex;
			optr1.gainRight = rightIndex;
			//
			optr1.gainSource = leftIndex;
			optr1.gainDest = rightIndex;
			optr1.gainLeftCentered = gainLeftCentered;
			optr1.gainRightCentered = gainRightCentered;
			optr1.gainDirec = (optr1.gainLeft > optr1.gainDest);
			break;
		}
	 case 2:
		{
			optr2.gainLeft = leftIndex;
			optr2.gainRight = rightIndex;
			//
			optr2.gainSource = leftIndex;
			optr2.gainDest = rightIndex;
			optr2.gainLeftCentered = gainLeftCentered;
			optr2.gainRightCentered = gainRightCentered;
			optr2.gainDirec = (optr2.gainLeft > optr2.gainDest);
			break;
		}
	 case 3:
		{
			optr3.gainLeft = leftIndex;
			optr3.gainRight = rightIndex;
			//
			optr3.gainSource = leftIndex;
			optr3.gainDest = rightIndex;
			optr3.gainLeftCentered = gainLeftCentered;
			optr3.gainRightCentered = gainRightCentered;
			optr3.gainDirec = (optr3.gainLeft > optr3.gainDest);
			break;
		}
	 case 4:
		{
			voice4_gainLeft = leftIndex;
			voice4_gainRight = rightIndex;
			break;
		}
	 default:
		{
			break;
		}
	}
}


uword sidEmu::sidEmuReturnVoiceVolume( int voice )
{
	uword left = 0;
	uword right = 0;
	switch ( voice )
	{
	 case 1:
		{
			left = optr1.gainLeft;
			right = optr1.gainRight;
			break;
		}
	 case 2:
		{
			left = optr2.gainLeft;
			right = optr2.gainRight;
			break;
		}
	 case 3:
		{
			left = optr3.gainLeft;
			right = optr3.gainRight;
			break;
		}
	 case 4:
		{
			left = voice4_gainLeft;
			right = voice4_gainRight;
			break;
		}
	 default:
		{
			break;
		}
	}
	return (uword)(left&0xff00)|(right>>8);
}


sidEmu::sidEmu(emuEngine* aEmuEngine)
/**
 * C'tor
 */
	:calls(50)
	,fastForwardFactor(128)
	,sidtuneClockSpeed(985248)
	,C64_clockSpeed(985248)
	,filterEnabled(true)
	,filterType(0)
	,filterCurType(0)
	,iTheEmuEngine(aEmuEngine)
	{
	CTOR(sidEmu);

	triangleTable = new ubyte[4096];
	sawtoothTable = new ubyte[4096];
	squareTable   = new ubyte[2*4096];

#ifdef LESS_CODE_BUT_ABIT_SLOWER
	waveform50 = new ubyte[1*4096]; // first half is data, last half is only zeros. Don't need to store these
#else
	waveform50 = new ubyte[2*4096];
#endif

#if defined(LARGE_NOISE_TABLE)
	noiseTableMSB = new ubyte[1<<8];
	noiseTableLSB = new ubyte[1L<<16];
#else
	noiseTableMSB = new ubyte[1<<8];
	noiseTableMID = new ubyte[1<<8];
	noiseTableLSB = new ubyte[1<<8];
#endif

    // MOS-8580, MOS-6581 (no 70)
	sidModeNormalTable[0]  = &sidEmu::sidMode00;
	sidModeNormalTable[1]  = &sidEmu::sidMode10;
	sidModeNormalTable[2]  = &sidEmu::sidMode20;
	sidModeNormalTable[3]  = &sidEmu::sidMode30;
	sidModeNormalTable[4]  = &sidEmu::sidMode40;
	sidModeNormalTable[5]  = &sidEmu::sidMode50;
	sidModeNormalTable[6]  = &sidEmu::sidMode60;
	sidModeNormalTable[7]  = &sidEmu::sidMode70;
	sidModeNormalTable[8]  = &sidEmu::sidMode80;
	sidModeNormalTable[9]  = &sidEmu::sidModeLock;
	sidModeNormalTable[10] = &sidEmu::sidModeLock;
	sidModeNormalTable[11] = &sidEmu::sidModeLock;
	sidModeNormalTable[12] = &sidEmu::sidModeLock;
	sidModeNormalTable[13] = &sidEmu::sidModeLock;
	sidModeNormalTable[14] = &sidEmu::sidModeLock;
	sidModeNormalTable[15] = &sidEmu::sidModeLock;
	
    // MOS-8580, MOS-6581 (no 74)
	sidModeRingTable[0]  = &sidEmu::sidMode00;
	sidModeRingTable[1]  = &sidEmu::sidMode14;
	sidModeRingTable[2]  = &sidEmu::sidMode00;
	sidModeRingTable[3]  = &sidEmu::sidMode34;
	sidModeRingTable[4]  = &sidEmu::sidMode00;
	sidModeRingTable[5]  = &sidEmu::sidMode54;
	sidModeRingTable[6]  = &sidEmu::sidMode00;
	sidModeRingTable[7]  = &sidEmu::sidMode74;
	sidModeRingTable[8]  = &sidEmu::sidModeLock;
	sidModeRingTable[9]  = &sidEmu::sidModeLock;
	sidModeRingTable[10] = &sidEmu::sidModeLock;
	sidModeRingTable[11] = &sidEmu::sidModeLock;
	sidModeRingTable[12] = &sidEmu::sidModeLock;
	sidModeRingTable[13] = &sidEmu::sidModeLock;
	sidModeRingTable[14] = &sidEmu::sidModeLock;
	sidModeRingTable[15] = &sidEmu::sidModeLock;

	iTheEnvelope = new Envelope();
	iTheSampler = new Sample(aEmuEngine->iThe6510);

// clear the memory
	memset(&optr1, sizeof(sidOperator), 0);
	memset(&optr2, sizeof(sidOperator), 0);
	memset(&optr3, sizeof(sidOperator), 0);
	
	iThe6510 = aEmuEngine->iThe6510;
	c64mem1 = iThe6510->c64mem1;
	c64mem2 = iThe6510->c64mem2;
	
	}


sidEmu::~sidEmu()
/**
 * D'tor
 */
	{
    DTOR(sidEmu);

	delete triangleTable;
	delete sawtoothTable;
	delete squareTable;

	delete waveform50;
	
#if defined(LARGE_NOISE_TABLE)
	delete noiseTableMSB;
	delete noiseTableLSB;
#else
	delete noiseTableMSB;
	delete noiseTableMID;
	delete noiseTableLSB;
#endif

	delete iTheEnvelope;
	delete iTheSampler;
	
	}


