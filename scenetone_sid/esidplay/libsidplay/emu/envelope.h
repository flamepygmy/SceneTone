//
// 1997/05/30 13:36:14
//

#ifndef ENVELOPE_H
#define ENVELOPE_H

//#include "mytypes.h"
#include "opstruct.h"
#include "epocglue.h"

static const ubyte ENVE_STARTATTACK = 0;
static const ubyte ENVE_STARTRELEASE = 2;

static const ubyte ENVE_ATTACK = 4;
static const ubyte ENVE_DECAY = 6;
static const ubyte ENVE_SUSTAIN = 8;
static const ubyte ENVE_RELEASE = 10;
static const ubyte ENVE_SUSTAINDECAY = 12;
static const ubyte ENVE_MUTE = 14;

static const ubyte ENVE_STARTSHORTATTACK = 16;
static const ubyte ENVE_SHORTATTACK = 16;

static const ubyte ENVE_ALTER = 32;

#if defined(SID_LINENVE)
static const udword attackTabLen = 255;
#endif

/**
 * Envelope class
 */
#if defined (__SYMBIAN32__)
class Envelope : public CBase
#else
class Envelope
#endif
	{
	friend class sidEmu;
	
  public:    // methods
	Envelope();
	virtual ~Envelope();
	//
	void enveEmuInit( udword updateFreq, bool measuredValues );
	void enveEmuResetOperator(struct sidOperator* pVoice);
	const ptr2sidUwordFunc* getEnveModeTable() const;

//  private:   // methods TODO
	inline uword enveEmuStartAttack(struct sidOperator*) const;
	inline uword enveEmuStartDecay(struct sidOperator*) const;
	inline uword enveEmuStartRelease(struct sidOperator*) const;
	inline uword enveEmuAlterAttack(struct sidOperator*) const;
	inline uword enveEmuAlterDecay(struct sidOperator*) const;
	inline uword enveEmuAlterSustain(struct sidOperator*) const;
	inline uword enveEmuAlterSustainDecay(struct sidOperator*) const;
	inline uword enveEmuAlterRelease(struct sidOperator*) const;
	inline uword enveEmuAttack(struct sidOperator*) const;
	inline uword enveEmuDecay(struct sidOperator*) const;
	inline uword enveEmuSustain(struct sidOperator*) const;
	inline uword enveEmuSustainDecay(struct sidOperator*) const;
	inline uword enveEmuRelease(struct sidOperator*) const;
	inline uword enveEmuMute(struct sidOperator*) const;

	inline uword enveEmuStartShortAttack(struct sidOperator*) const;
	inline uword enveEmuAlterShortAttack(struct sidOperator*) const;
	inline uword enveEmuShortAttack(struct sidOperator*) const;

  public:    // members
	ubyte masterVolume;
	uword masterVolumeAmplIndex;
  private:   // members
	uword masterAmplModTable[16*256];

#ifdef SID_FPUENVE
	float attackRates[16];
	float decayReleaseRates[16];  
#elif defined(DIRECT_FIXPOINT)
	udword attackRates[16];
	udword decayReleaseRates[16];
#else
	udword attackRates[16];
	udword attackRatesP[16];
	udword decayReleaseRates[16];  
	udword decayReleaseRatesP[16];  
#endif


#if !defined(SID_LINENVE)
	udword attackTabLen;
	udword attackPos[256];
#endif

	udword releaseTabLen;
	udword releasePos[256];
 
	static const ptr2sidUwordFunc enveModeTable[32];
	
	}; // end, class

#endif
