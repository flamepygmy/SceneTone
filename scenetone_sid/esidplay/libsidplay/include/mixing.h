//
// mixing.h
//
#if !defined(__MIXING_H__)
#define __MIXING_H__

#include "opstruct.h"
//#include "epocglue.h"
#include "6581_.h"

//
// constants
//
static const int maxLogicalVoices = 4;
static const int mix8monoMiddleIndex = 256*maxLogicalVoices/2;
static const int mix8stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;
static const int mix16monoMiddleIndex = 256*maxLogicalVoices/2;
static const int mix16stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;


//
// class Mixer
//
class Mixer// : public sidEmuBase
	{

	friend class emuEngine;
	
  public:
	Mixer(emuEngine* aEmuEngine, sidEmu* aSidEmu);
	~Mixer();
	void MixerInit(bool threeVoiceAmplify, ubyte zero8, uword zero16);
	inline void syncEm();
	inline void* fillFunc(void* buffer, udword bufferLen) {	return (this->*sidEmuFillFunc)(buffer, bufferLen); }
	
  public:
	void* fill8bitMono( void* buffer, udword numberOfSamples );
	void* fill8bitMonoControl( void* buffer, udword numberOfSamples );
	void* fill8bitStereo( void* buffer, udword numberOfSamples );
	void* fill8bitStereoControl( void* buffer, udword numberOfSamples );
	void* fill8bitStereoSurround( void* buffer, udword numberOfSamples );
	void* fill8bitsplit( void* buffer, udword numberOfSamples );
	void* fill16bitMono( void* buffer, udword numberOfSamples );
	void* fill16bitMonoControl( void* buffer, udword numberOfSamples );
	void* fill16bitStereo( void* buffer, udword numberOfSamples );
	void* fill16bitStereoControl( void* buffer, udword numberOfSamples );
	void* fill16bitStereoSurround( void* buffer, udword numberOfSamples );
	void* fill16bitsplit( void* buffer, udword numberOfSamples );
  public:
//void* fill8bitMono(void*, udword); // only need one fill()-prototype here
//	void* (*sidEmuFillFunc)(void*, udword); // default
	void* (Mixer::*sidEmuFillFunc)(void*, udword); // default

  private:
	sidEmu* iTheSidEmu;
    //
	ubyte mix8mono[256*maxLogicalVoices];
	ubyte mix8stereo[256*(maxLogicalVoices/2)];
	uword mix16mono[256*maxLogicalVoices];
	uword mix16stereo[256*(maxLogicalVoices/2)];

	sbyte *signedPanMix8;
	sword *signedPanMix16;

	ubyte zero8bit;   // ``zero''-sample
	uword zero16bit;  // either signed or unsigned
	udword splitBufferLen;

	Sample* iTheSampler;
	
//	sidOperator& optr1, optr2, optr3;          // -> 6581_.cc
//	uword& voice4_gainLeft, voice4_gainRight;
	
	};



#endif // MIXING
