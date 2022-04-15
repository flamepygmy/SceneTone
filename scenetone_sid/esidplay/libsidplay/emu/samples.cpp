//
// 1997/10/01 23:55:11
// 

#include "samples.h"
#include "6581_.h"
#include "6510_.h"


//extern ubyte* c64mem1;
//extern ubyte* c64mem2;




// ---


//
// constant data
//

// Sample Order Modes.
static const ubyte SO_LOWHIGH = 0;
static const ubyte SO_HIGHLOW = 1;

static const sbyte galwayNoiseTab1[16] =
{
	0x80u,0x91u,0xa2u,0xb3u,0xc4u,0xd5u,0xe6u,0xf7u,
	0x08u,0x19u,0x2au,0x3bu,0x4cu,0x5du,0x6eu,0x7fu
};

static const sbyte sampleConvertTab[16] =
{
//  0x81,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,
//  0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x7f
	0x81u,0x90u,0xa0u,0xb0u,0xc0u,0xd0u,0xe0u,0xf0u,
	0x00u,0x10u,0x20u,0x30u,0x40u,0x50u,0x60u,0x70u
};


//
// implementation of Sample
//

Sample::Sample(C6510* a6510)
/**
 * C'tor
 */
	{
	CTOR(Sample);

	sampleEmuRout = &Sample::sampleEmuSilence;

	c64mem1 = a6510->c64mem1;
	c64mem2 = a6510->c64mem2;
	}


Sample::~Sample()
/**
 * D'tor
 */
	{
	DTOR(Sample);
	}


void Sample::channelReset(sampleChannel& ch)
{
	ch.Active = false;
	ch.Mode = FM_NONE;
	ch.Period = 0;
#if defined(DIRECT_FIXPOINT)
	ch.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch.Pos_stp = (ch.Pos_pnt = 0);
#else
	ch.Pos_stp = 0;
#endif
}

inline void Sample::channelFree(sampleChannel& ch, const uword regBase)
	{
	ch.Active = false;
	ch.Mode = FM_NONE;
	c64mem2[regBase+0x1d] = 0x00;
	}

inline void Sample::channelTryInit(sampleChannel& ch, const uword regBase)
{
	if ( ch.Active && ( ch.Mode == FM_GALWAYON ))
		return;

	ch.VolShift = ( 0 - (sbyte)c64mem2[regBase+0x1d] ) >> 1;
	c64mem2[regBase+0x1d] = 0x00;
	ch.Address = readLEword(c64mem2+regBase+0x1e);
	ch.EndAddr = readLEword(c64mem2+regBase+0x3d);
	if (ch.EndAddr <= ch.Address)
	{
		return;
	}
	ch.Repeat = c64mem2[regBase+0x3f];
	ch.RepAddr = readLEword(c64mem2+regBase+0x7e);
	ch.SampleOrder = c64mem2[regBase+0x7d];
	uword tempPeriod = readLEword(c64mem2+regBase+0x5d);
	if ( (ch.Scale=c64mem2[regBase+0x5f]) != 0 )
	{
		tempPeriod >>= ch.Scale;
	}
	if ( tempPeriod == 0 )
	{
		ch.Period = 0;
#if defined(DIRECT_FIXPOINT) 
		ch.Pos_stp.l = (ch.PosAdd_stp.l = 0);
#elif defined(PORTABLE_FIXPOINT)
		ch.Pos_stp = (ch.Pos_pnt = 0);
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.Pos_stp = (ch.PosAdd_stp = 0);
#endif
		ch.Active = false;
		ch.Mode = FM_NONE;
	}
	else
	{ 	  
		if ( tempPeriod != ch.Period ) 
		{
			ch.Period = tempPeriod;
#if defined(DIRECT_FIXPOINT) 
			ch.Pos_stp.l = sampleClock / ch.Period;
#elif defined(PORTABLE_FIXPOINT)
			udword tempDiv = sampleClock / ch.Period;
			ch.Pos_stp = tempDiv >> 16;
			ch.Pos_pnt = tempDiv & 0xFFFF;
#else
			ch.Pos_stp = sampleClock / ch.Period;
#endif
		}
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		ch.Active = true;
		ch.Mode = FM_HUELSON;
	}
}

inline ubyte Sample::channelProcess(sampleChannel& ch, const uword regBase)
{
#if defined(DIRECT_FIXPOINT) 
	uword sampleIndex = ch.PosAdd_stp.w[HI] + ch.Address;
#elif defined(PORTABLE_FIXPOINT)
	uword sampleIndex = ch.PosAdd_stp + ch.Address;
#else
	uword sampleIndex = (ch.PosAdd_stp >> 16) + ch.Address;
#endif
    if ( sampleIndex >= ch.EndAddr )
	{
		if ( ch.Repeat != 0xFF )
		{ 
			if ( ch.Repeat != 0 )  
				ch.Repeat--;
			else
			{
				channelFree(ch,regBase);
				return 8;
			}
		}
		sampleIndex = ( ch.Address = ch.RepAddr );
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		if ( sampleIndex >= ch.EndAddr )
		{
			channelFree(ch,regBase);
			return 8;
		}
	}  
  
	ubyte tempSample = c64mem1[sampleIndex];
	if (ch.SampleOrder == SO_LOWHIGH)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if (ch.PosAdd_stp.w[LO] >= 0x8000)
#elif defined(PORTABLE_FIXPOINT)
			if ( ch.PosAdd_pnt >= 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) != 0 )
#endif
			{
			    tempSample >>= 4;
			}
		}
		// AND 15 further below.
	}
	else  // if (ch.SampleOrder == SO_HIGHLOW)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if ( ch.PosAdd_stp.w[LO] < 0x8000 )
#elif defined(PORTABLE_FIXPOINT)
		    if ( ch.PosAdd_pnt < 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) == 0 )
#endif
			{
			    tempSample >>= 4;
			}
			// AND 15 further below.
		}
		else  // if (ch.Scale != 0)
		{
			tempSample >>= 4;
		}
	}
	
#if defined(DIRECT_FIXPOINT) 
	ch.PosAdd_stp.l += ch.Pos_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch.PosAdd_pnt + (udword)ch.Pos_pnt;
	ch.PosAdd_pnt = temp & 0xffff;
	ch.PosAdd_stp += ( ch.Pos_stp + ( temp > 65535 ));
#else
	ch.PosAdd_stp += ch.Pos_stp;
#endif
	
	return (tempSample&0x0F);
}

// ---

void Sample::sampleEmuReset(sidEmu* aTheSidEmu)
/**
 * reset some important variables
 */
{
	channelReset(ch4);
	channelReset(ch5);
//	sampleClock = (udword) (((C64_clockSpeed / 2.0) / PCMfreq) * 65536UL); // original - calls __fixunsdfsi
//	sampleClock = (sdword) (((C64_clockSpeed / 2.0) / PCMfreq) * 65536UL); //ALFRED - 
	sampleClock = (udword) (((aTheSidEmu->C64_clockSpeed / 2) / aTheSidEmu->PCMfreq) * 65536L);
	sampleEmuRout = &Sample::sampleEmuSilence;
	if ( c64mem2 != 0 )
	{
		channelFree(ch4,0xd400);
		channelFree(ch5,0xd500);
	}
}

	;      

void Sample::sampleEmuInit(sidEmu* aTheSidEmu)
/**
 * precalculate tables + reset
 */
{
	int k = 0;
	for ( int i = 0; i < 16; i++ )
	{
		int l = 0;
		for ( int j = 0; j < 64; j++ )
		{
			galwayNoiseTab2[k++] = galwayNoiseTab1[l];
			l = (l+i) & 15;
		}
	}
	sampleEmuReset(aTheSidEmu);
}

sbyte Sample::sampleEmuSilence()
{
	return 0;
}

inline void Sample::sampleEmuTryStopAll()
{
	if ( !ch4.Active && !ch5.Active )
	{
		sampleEmuRout = &Sample::sampleEmuSilence;
	}
}

void Sample::sampleEmuCheckForInit()
{
	// Try first sample channel.
	switch ( c64mem2[0xd41d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch4,0xd400);
		break;
	 case 0xFD:
		channelFree(ch4,0xd400);
		break;
	 case 0xFC:
		channelTryInit(ch4,0xd400);
		break;
	 case 0x00:
		break;
	 default:
		GalwayInit();
		break;
	}
	
	if (ch4.Mode == FM_HUELSON)
	{
		sampleEmuRout = &Sample::sampleEmu;
	}
	
	// Try second sample channel.
	switch ( c64mem2[0xd51d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch5,0xd500);
		break;
	 case 0xFD:
		channelFree(ch5,0xd500);
		break;
	 case 0xFC:
		channelTryInit(ch5,0xd500);
		break;
	 default:
		break;
	}
	
	if (ch5.Mode == FM_HUELSON)
	{
		sampleEmuRout = &Sample::sampleEmuTwo;
	}
	sampleEmuTryStopAll();
}

sbyte Sample::sampleEmu()
{
	// One sample channel. Return signed 8-bit sample.
	if ( !ch4.Active )
		return 0;
	else
		return (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
}

sbyte Sample::sampleEmuTwo()
{
	sbyte sample = 0;
	if ( ch4.Active )
		sample += (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
	if ( ch5.Active )
		sample += (sampleConvertTab[channelProcess(ch5,0xd500)]>>ch5.VolShift);
	return sample;
}

// ---
  
inline void Sample::GetNextFour()
{
	uword tempMul = (uword)(c64mem1[ch4.Address+(uword)ch4.Counter])
		* ch4.LoopWait + ch4.NullWait;
	ch4.Counter--;
#if defined(DIRECT_FIXPOINT)
	if ( tempMul != 0 )
		ch4.Period_stp.l = sampleClock / tempMul;
	else
		ch4.Period_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	udword tempDiv; 
	if ( tempMul != 0 )
		tempDiv = sampleClock / tempMul;
	else
		tempDiv = 0;
	ch4.Period_stp = tempDiv >> 16;
	ch4.Period_pnt = tempDiv & 0xFFFF;
#else
	if ( tempMul != 0 )
		ch4.Period_stp = sampleClock / tempMul;
	else
		ch4.Period_stp = 0;
#endif
	ch4.Period = tempMul;
}

void Sample::GalwayInit()
{
	if (ch4.Active) 
		return;
	
	sampleEmuRout = &Sample::sampleEmuSilence;
  
	ch4.Counter = c64mem2[0xd41d];  
	c64mem2[0xd41d] = 0; 
  
	if ((ch4.Address=readLEword(c64mem2+0xd41e)) == 0)
		return;
  
	if ((ch4.LoopWait=c64mem2[0xd43f]) == 0)
		return;
  
	if ((ch4.NullWait=c64mem2[0xd45d]) == 0)
		return;
  
	if (c64mem2[0xd43e] == 0)
		return;
	ch4.SamAddr = ((uword)c64mem2[0xd43e]&15) << 6;
  
	if ( c64mem2[0xd43d] == 0 )
		return;
	ch4.SamLen = (((uword)c64mem2[0xd43d])+1) & 0xfffe;
  
	ch4.Active = true;
	ch4.Mode = FM_GALWAYON;
  
	//GalwayFourStart:
#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch4.Pos_stp = 0;
	ch4.Pos_pnt = 0;
#else
	ch4.Pos_stp = 0;
#endif
	//SelectNewVolume();
	GetNextFour();
	ch4.Counter++;
  
	sampleEmuRout = &Sample::GalwayReturnSample;
}

sbyte Sample::GalwayReturnSample()
{
#if defined(DIRECT_FIXPOINT)
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + (ch4.Pos_stp.w[HI] & 15) ];
#elif defined(PORTABLE_FIXPOINT)
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + ( ch4.Pos_stp & 15 )];
#else
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + ((ch4.Pos_stp >> 16) & 15) ];
#endif
	
#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l += ch4.Period_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch4.Pos_pnt;
	temp += (udword)ch4.Period_pnt;
	ch4.Pos_pnt = temp & 0xffff;
	ch4.Pos_stp += ( ch4.Period_stp + ( temp > 65535 ));
#else
	ch4.Pos_stp += ch4.Period_stp;
#endif

#if defined(DIRECT_FIXPOINT)
	if ( ch4.Pos_stp.w[HI] >= ch4.SamLen )
#elif defined(PORTABLE_FIXPOINT)
	if ( ch4.Pos_stp >= ch4.SamLen )
#else
	if ( (ch4.Pos_stp >> 16) >= ch4.SamLen )
#endif
	{
#if defined(DIRECT_FIXPOINT)
		ch4.Pos_stp.w[HI] = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch4.Pos_stp = 0;
#else
		ch4.Pos_stp &= 0xFFFF;
#endif
	//GalwayFour:
	GetNextFour();
	if ( ch4.Counter == 0xff )  {
	  //GalwayFourEnd:
	  //SelectVolume();
	  ch4.Active = false;
	  ch4.Mode = FM_GALWAYOFF;
	  sampleEmuRout = &Sample::sampleEmuSilence;
	}
  }
  return tempSample;
}

//static void SelectVolume()
//{
//	(c64mem2[0xd418] & 15) << 2;
//}

//static void SelectNewVolume()
//{
//	if (( c64mem2[0xd418] & 15 ) < 12 )
//	{
//	}
//}
