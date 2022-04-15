//
// 6581_.h
//
#if !defined(__6581_H__)
#define __6581_H__

#include "opstruct.h"
#include "epocglue.h"

//
// forward declarations
//
class emuEngine;
class sidTune;
class Mixer;
class Envelope;
class Sample;
class C6510;


/**
 * implements MOS-6581 functionality
 * aka 'the SID-chip'!
 */
class sidEmu
	{

	friend class emuEngine;
	friend class Mixer;
	friend class Sample;
	
  public:
	sbyte waveCalcMute(struct sidOperator* pVoice);
	sbyte waveCalcNormal(struct sidOperator* pVoice);
	sbyte waveCalcRangeCheck(struct sidOperator* pVoice);

	void sidMode00(struct sidOperator* pVoice);
	void sidMode10(struct sidOperator* pVoice);
	void sidMode20(struct sidOperator* pVoice);
	void sidMode30(struct sidOperator* pVoice);
	void sidMode40(struct sidOperator* pVoice);
	void sidMode50(struct sidOperator* pVoice);
	void sidMode60(struct sidOperator* pVoice);
	void sidMode70(struct sidOperator* pVoice);
	void sidMode80(struct sidOperator* pVoice);
	void sidMode80hp(struct sidOperator* pVoice);
	void sidModeLock(struct sidOperator* pVoice);
	void sidMode14(struct sidOperator* pVoice);
	void sidMode34(struct sidOperator* pVoice);
	void sidMode54(struct sidOperator* pVoice);
	void sidMode74(struct sidOperator* pVoice);

  protected: // methods
	inline void noiseAdvance(struct sidOperator* pVoice);
	inline void noiseAdvanceHp(struct sidOperator* pVoice);

  public:
	sidEmu(emuEngine* aEmuEngine);
	virtual ~sidEmu();
	//
	inline void calcValuesPerCall();	
	void sidEmuSetReplayingSpeed(int clockMode, uword callsPerSec);
	inline void sidEmuSet(struct sidOperator* pVoice, uword sidIndex);
	inline void waveCalcFilter(struct sidOperator* pVoice);
	inline void sidEmuSet2(struct sidOperator* pVoice);
	void sidEmuFillBuffer(emuEngine& thisEmu, sidTune& thistune, void* buffer, udword bufferLen);
	bool sidEmuFastForwardReplay( int percent );
	void sidEmuConfigure(const struct emuConfig& aEmuConfig);
	
//	void sidEmuConfigure(udword PCMfrequency, bool measuredEnveValues, bool isNewSID, bool emulateFilter, int clockSpeed);
	void initWaveformTables(bool isNewSID);
	bool sidEmuReset();
	void clearSidOperator( struct sidOperator* pVoice );
	void sidEmuResetAutoPanning(int autoPanning);
	void sidEmuSetVoiceVolume(int voice, uword leftLevel, uword rightLevel, uword total);
	uword sidEmuReturnVoiceVolume( int voice );
	

// ------------ unknowns:
	
	void sidEmuUpdateReplayingSpeed();

//  protected:
	void sidEmuChangeReplayingSpeed();
	void sidEmuSetClockSpeed(int clockMode);

  public:
	struct sidOperator optr1, optr2, optr3;
	uword voice4_gainLeft, voice4_gainRight; ///< Voice 4 does not use a sidOperator structure.
	udword splitBufferLen;                   ///< shared with Mixer class
	
  private:

    // tables
	float filterTable[0x800];
	float bandPassParam[0x800];
	float filterResTable[16];

	ptr2sidVoidFunc sidModeNormalTable[16]; ///< MOS-8580, MOS-6581 (no 70)
	ptr2sidVoidFunc sidModeRingTable[16];   ///< MOS-8580, MOS-6581 (no 74)

	float filterDy, filterResDy;
	float C64_fClockSpeed;      ///< we don't need to store the default value here

	uword calls;                ///< calls per second (here a default)
	uword filterValue;
	uword apCount;
	uword fastForwardFactor;    ///< normal speed
	uword  PCMfreq;
	udword sidtuneClockSpeed;   ///< Song clock speed (PAL or NTSC). Does not affect pitch.
	udword PCMsid;
	udword PCMsidNoise;
	udword C64_clockSpeed;      ///< Master clock speed. Affects pitch of SID and CIA samples.

	bool filterEnabled;
	bool updateAutoPanning;
	bool doAutoPanning;

	ubyte filterType;
	ubyte filterCurType;

#if defined(DIRECT_FIXPOINT)
    cpuLword VALUES, VALUESadd, VALUESorg;
#else
    uword VALUES, VALUESorg;
    udword VALUESadd, VALUEScomma;
#endif

	uword toFill;

#if defined(SIDEMU_TIME_COUNT)
	udword prevBufferLen;    // need for fast_forward time count
	udword scaledBufferLen;
#endif

	uword defaultTimer;
	uword timer;

	Sample* iTheSampler; ///< pointer to the sample. We own this.
	
    // always pointer first:
	ubyte* triangleTable;
	ubyte* sawtoothTable;
	ubyte* squareTable;
	ubyte* waveform50;

#if defined(LARGE_NOISE_TABLE)
	ubyte* noiseTableMSB;
	ubyte* noiseTableLSB;
#else
	ubyte* noiseTableMSB;
	ubyte* noiseTableMID;
	ubyte* noiseTableLSB;
#endif

	Envelope* iTheEnvelope;
	emuEngine* iTheEmuEngine;
	C6510* iThe6510;
	
	// these buffers are owned by C6510
	ubyte* c64mem1;       ///< pointer to 64KB C64-RAM owned by C6510
	ubyte* c64mem2;       ///< pointer to Basic-ROM, VIC, SID, I/O, Kernal-ROM owned by C6510

	}; // end, class sidEmu


#endif // __6581_H__
