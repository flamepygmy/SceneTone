/**
 * @file audiodrv_epoc.h Defines the class audioDriver, a thin wrapper around the
 * architecture specific sound driver. This file is based on the Linux version of 'audiodrv.h' by Michael Schwendt
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */
#ifndef AUDIODRV_H
#define AUDIODRV_H


#include <e32std.h>

#ifdef __ER6__
#include <mdasound.h>
#else
#include <d32snd.h>
#endif

#include "mytypes.h"
#include "emucfg.h"


/**
 * This class is another audio driver abstraction, a thin wrapper around the
 * architecture specified sound driver. In this case it interfaces the
 * RDevSound class available in the ER5 version of EPOC32.
 *
 */
class audioDriver
	{
public:
	audioDriver();
	~audioDriver();
	bool IsThere();
	bool Open(udword freq, int precision, int channels, int fragments, int fragBase);
	void Close();
	void Play(ubyte* buffer, int bufferSize);
	TInt VolumeDelta(TInt aDelta);

	bool Reset()
	{
	return 0; //TODO - ALFRED
	}

	int GetAudioHandle(){return audioHd;}
	udword GetFrequency(){return frequency;}
	int GetChannels(){return channels;}
	int GetSamplePrecision(){return precision;}
	int GetSampleEncoding(){return encoding;}
	int GetBlockSize(){return blockSize;}
	int GetFragments(){return fragments;}
	int GetFragSizeBase(){return fragSizeBase;}
	const char* GetErrorString(){return errorString;}

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

#ifdef __ER6__
	RMdaDevSound iDevSound;
#else
	// Epoc specific stuff
	RDevSound iDevSound;
	HBufC8* iAlawBuffer;
#endif

};


#endif  // AUDIODRV_H
