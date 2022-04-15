/**
 * @file audiodrv_er6.h Defines the class audioDriver, a thin wrapper around the
 * architecture specific sound driver, in this case Epoc Release 6 (ER6)
 * This file is based on the Linux version of 'audiodrv.h' by Michael Schwendt
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */
#ifndef AUDIODRV_ER6_H
#define AUDIODRV_ER6_H

#include <e32def.h>
#include <e32std.h>
#include <mdaaudiooutputstream.h>
#include <mda/common/audio.h>
#include "mytypes.h"
#include "emucfg.h"
#include <f32file.h>

/**
 * a word about buffer size...
 *
 * too big, and it causes KErrUnderrun
 * too small, gives to much overhead
 *
 * 0x10000 should be enough
 * 0x40000 is too much
 */
const TInt KDefaultBufSize = 0x2000; /// @todo determine size
//#define SAMPLE_FREQ 44100 ///< in Hertz
#define SAMPLE_FREQ 8000 ///< in Hertz
const TInt KVolumeSteps = 16; ///< Number of steps for volume control


/**
 * This class is another audio driver abstraction, a thin wrapper around the
 * architecture specified sound driver. In this case it interfaces the
 * CMdaAudioOutputStream class available in the ER6+ version of SymbianOS.
 *
 */
class audioDriver : public MMdaAudioOutputStreamCallback, public CBase
	{
public:
	audioDriver();
	void ConstructL();
	~audioDriver();
	bool IsThere();
	bool Open(udword freq, int precision, int channels, int fragments, int fragBase);
	void Close();
	void Play(ubyte* buffer, int bufferSize);
	void StopStream();
	TInt VolumeDelta(TInt aDelta);

	// from MMdaAudioOutputStreamCallback
	virtual void MaoscOpenComplete(TInt aError);
	virtual void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
	virtual void MaoscPlayComplete(TInt aError);

	// methods for dumping to WAV file
public:
	TInt StartWavDump(const TDesC& aSidTune);
	TInt StopWavDump();
	TBool IsWavDumping();
private:
	void DoWavDump(const TDesC8& aBuffer);

public:
	bool Reset() {return 0;}
	int GetAudioHandle(){return audioHd;}
	udword GetFrequency(){return frequency;}
	int GetChannels(){return channels;}
	int GetSamplePrecision(){return precision;}
	int GetSampleEncoding(){return encoding;}
	int GetBlockSize(){return blockSize;}
	int GetFragments(){return fragments;}
	int GetFragSizeBase(){return fragSizeBase;}
	const char* GetErrorString(){return errorString;}
private:
	void ResetStream();

private:  // ------------------------------------------------------- private
	int audioHd;
	const char* errorString;
	int blockSize;
	int fragments;
	int fragSizeBase;
	udword frequency;

	// These are constants/enums from ``libsidplay/include/emucfg.h''.
	int encoding;
	int precision;
	int channels;

	TInt iVolume;
	CMdaAudioOutputStream* iMdaAudio;    ///< pointer to audio stream
	TMdaAudioDataSettings iSettings;     ///< audio settings
	TBuf8<KDefaultBufSize*2> iExtraBuf;  ///< extra double buffering
public:
	TBool iIsReady;                      ///< true if audio device is opened
	TInt iBlocksInQueue;                 ///< number of blocks in audio stream queue
	TBool iPrefilled;                    ///< set to true after FillBuffer is called
private:
	RFs iFs;                             ///< Handle to fileserver for wav dumping
	TFileName iWavDump;                  ///< Filename of wavefile
	TBool iIsWavDumping;                 ///< True if wav dumping is active
	};


#endif  // AUDIODRV_ER6_H
