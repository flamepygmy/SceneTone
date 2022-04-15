// AUDIODRV.CPP
//
// (c) 2000 Alfred E. Heggestad
//
// This file is based on the Linux version of 'audiodrv.cpp' by Michael Schwendt
//

/**
 * @file audiodrv.cpp
 *
 * Implements the class audioDriver, a thin wrapper around the
 * architecture specified sound driver.
 */

#include "audiodrv_epoc.h"
#include "alaw.h"

const TInt KDefaultBufSize = 8000; //TODO: determine size


audioDriver::audioDriver()
/**
 * C'tor
 */
	:audioHd(-1)
	,frequency(0)
	,encoding(0)
	,precision(0)
	,channels(0)
	{
	CTOR(audioDriver);

	// Reset everything.
	errorString = "None";
#ifndef __ER6__
	iAlawBuffer = HBufC8::NewL(KDefaultBufSize);
#endif
	}


audioDriver::~audioDriver()
/**
 * D'tor
 */
	{
	DTOR(audioDriver);
#ifndef __ER6__
	delete iAlawBuffer;
#endif
	}


bool audioDriver::IsThere()
/**
 * Check device availability and write permissions.
 *
 * @return bool true if audio device is available
 */
	{

	TInt ret = iDevSound.Open();
	if(ret == KErrNone)
		{
		iDevSound.Close();
		return true;
		}
	return false;

	}


bool audioDriver::Open(udword inFreq, int inPrecision, int inChannels,
					   int inFragments, int inFragBase)
/**
 * Open the sound device. Returns true if success.
 *
 * @param inFreq Sampling frequency in Hz
 * @param inPrecision Number of bits pr. sample
 * @param inChannels Number of channels (Stereo=2)
 * @param inFragments number of audio buffer fragments to use
 * @param inFragBase size 2^<num> of audio buffer fragments
 * @return true if success, false if errors
 */
	{
	TInt ret = iDevSound.Open();

	if ( ret != KErrNone )
		{
		errorString = "AUDIO: Could not open audio device.";
		return false;
		}

	audioHd	= KErrNone; // means OK and opened

	// Transfer input parameters to this object.
	// May later be replaced with driver defaults.
	frequency = inFreq;
	precision = inPrecision;
	channels = inChannels;
	fragments = inFragments;
	fragSizeBase = inFragBase;

	// Set sample precision and type of encoding. (only 8 bit for EPOC)
//	int dsp_sampleSize = 8;
#ifdef __ER6__
	if (precision != SIDEMU_8BIT && precision != SIDEMU_16BIT )
#else
	if (precision != SIDEMU_8BIT)
#endif
		{
		errorString = "AUDIO: Could not set sample size.(only 8 bit for EPOC)";
		return false;
		}

//	precision = SIDEMU_8BIT;
	encoding = SIDEMU_UNSIGNED_PCM;

	if (channels != SIDEMU_MONO)
		{
		errorString = "AUDIO: only Mono supported for EPOC.";
		return false;
		}

	// Set mono/stereo.
	int dsp_stereo;
	if (channels == SIDEMU_STEREO)
		dsp_stereo = 1;
	else if (channels == SIDEMU_MONO)
		dsp_stereo = 0;
	else
		{
		errorString = "AUDIO: Could not set mono/stereo.";
		return false;
		}

	// Verify and accept the number of channels the driver accepted.
	if (dsp_stereo == 1)
		channels = SIDEMU_STEREO;
	else if (dsp_stereo == 0)
		channels = SIDEMU_MONO;
	else
		{
		errorString = "AUDIO: Could not set mono/stereo.";
		return false;
		}

	// Set frequency.
#ifdef __ER6__

	RMdaDevSound::TSoundFormatsSupportedBuf formats;
	iDevSound.PlayFormatsSupported(formats);

	_ELOG_P1(_L8("RMdaDevSound - formats:\n"));
	_ELOG_P2(_L8("  iMinRate       = %d\n"), formats().iMinRate);
	_ELOG_P2(_L8("  iMaxRate       = %d\n"), formats().iMaxRate);
	_ELOG_P2(_L8("  iEncodings     = 0x%08x\n"), formats().iEncodings);
	_ELOG_P2(_L8("  iChannels      = %d\n"), formats().iChannels);
	_ELOG_P2(_L8("  iMinBufferSize = %d\n"), formats().iMinBufferSize);
	_ELOG_P2(_L8("  iMaxBufferSize = %d\n"), formats().iMaxBufferSize);
	_ELOG_P2(_L8("  iMinVolume     = %d\n"), formats().iMinVolume);
	_ELOG_P2(_L8("  iMaxVolume     = %d\n"), formats().iMaxVolume);

	RMdaDevSound::TCurrentSoundFormatBuf format;
	format().iRate = frequency;
	if(precision==SIDEMU_8BIT)
		format().iEncoding = RMdaDevSound::EMdaSoundEncoding8BitPCM;
	else if(precision==SIDEMU_8BIT)
		format().iEncoding = RMdaDevSound::EMdaSoundEncoding16BitPCM;
	else
		{
		errorString = "AUDIO: unsupported encoding precision [precision]\n";
		_ELOG_P2(_L8("%s\n"), errorString);
		return false;
		}

	format().iChannels = channels;
	format().iBufferSize = 0x8000;
	if(iDevSound.SetPlayFormat(format) != KErrNone)
		{
		errorString = "AUDIO: could not set config\n";
		_ELOG_P2(_L8("AUDIO: Warning! Could not set config due to [%d]\n"), ret);
		return false;
		}

	blockSize = format().iBufferSize / 2;
	fragments = 2; //TODO: find a value for this

#else
	if (frequency != KAlawSamplesPerSecond)
		{
		errorString = "AUDIO: only frequency 8000 Hz supported for EPOC ER5.";
		return false;
		}

	//
	// get sound device capabilites
	//
	TSoundCaps sndCaps;
	iDevSound.Caps(sndCaps);

	_ELOG_P1(_L8("RDevSound - caps:\n"));
	_ELOG_P2(_L8("  iService      = 0x%08x\n"), sndCaps().iService);
	_ELOG_P2(_L8("  iVolume       = 0x%08x\n"), sndCaps().iVolume);
	_ELOG_P2(_L8("  iMaxVolume    = 0x%08x\n"), sndCaps().iMaxVolume);
	_ELOG_P2(_L8("  iMaxFrequency = 0x%08x\n"), sndCaps().iMaxFrequency);


	//
	// configure sound device
	//
	TSoundConfig sndConfig;
	iDevSound.Config(sndConfig); // read the default config

	sndConfig().iVolume = EVolumeByValue;
	sndConfig().iVolumeValue = sndCaps().iMaxVolume;
	sndConfig().iAlawBufferSize = KDefaultBufSize;

	ret = iDevSound.SetConfig(sndConfig); // read the default config
	if(ret != KErrNone)
		{
		errorString = "AUDIO: could not set config\n";
		_ELOG_P2(_L8("AUDIO: Warning! Could not set config due to [%d]\n"), ret);
		return false;
		}

	_ELOG_P2(_L8("AUDIO: iVolume         %d \n"), sndConfig().iVolume );
	_ELOG_P2(_L8("       iAlawBufferSize %d \n"), sndConfig().iAlawBufferSize );

	blockSize = KDefaultBufSize / 2;
	fragments = 2; //TODO: find a value for this

#endif // __ER6__


#ifndef __ER6__
	// prepare the buffer
	ret = iDevSound.PreparePlayAlawBuffer(); // This *must* be done before reading the Buffer size!!!
	if(ret != KErrNone)
		{
		errorString = "AUDIO: could not prepare alaw buffer\n";
		_ELOG_P2(_L8("AUDIO: Warning! could not prepare alaw buffer due to [%d]\n"), ret);
		return false;
		}
#endif

	return true; // success !
	}


void audioDriver::Close()
/**
 * Close an opened audio device, free any allocated buffers and
 * reset any variables that reflect the current state.
 */
	{
	if (audioHd != (-1))
		{
		iDevSound.Close();
		audioHd = (-1);
		}
	}

void audioDriver::Play(ubyte* pBuffer, int bufferSize)
/**
 * play a buffer with samples. Because of the strange alaw
 * API on the ER5 RDevSound class we have to convert the PCM
 * data into alaw before playing them. This is adding one
 * extra level of CPU eating overheader...
 *
 * @param pBuffer pointer to the buffer to be player
 * @param bufferSize number of bytes in the buffer
 */
	{
	if (audioHd != (-1))
		{

		TRequestStatus stat;

#ifdef __ER6__

		TPtrC8 ptr(pBuffer, bufferSize);
		iDevSound.PlayData(stat, ptr);

#else
		//
		// convert from linear PCM to aLaw first
		//
		TPtr8 alawPtr = iAlawBuffer->Des();
		alawPtr.SetLength(bufferSize);
		Alaw::conv_u8bit_alaw(pBuffer, &alawPtr[0], bufferSize);

		//
		// and then play the alaw buffer
		//
		iDevSound.PlayAlawData(stat, alawPtr);

#endif // __ER6__

		User::WaitForRequest(stat);
		TInt ret = stat.Int();
		if(ret != KErrNone)
			{
			_ELOG_P2(_L8("AUDIO: Warning! could not play due to %d\n"), ret);
			User::After(1000000);
			}

		}

	}


TInt audioDriver::VolumeDelta(TInt aDelta)
/**
 * increase or decrease the volume by aDelta, and return the new volume
 */
	{
#ifdef __ER6__
	RMdaDevSound::TSoundFormatsSupportedBuf formats;
	iDevSound.PlayFormatsSupported(formats);
	const TInt min = formats().iMinVolume;
	const TInt max = formats().iMaxVolume;
	const TInt steps = (max - min) / 16;

	const TInt new_volume = iDevSound.PlayVolume() + (aDelta*steps);
	if(new_volume > max)
		iDevSound.SetPlayVolume(max);
	else if(new_volume < min)
		iDevSound.SetPlayVolume(min);
	else
		iDevSound.SetPlayVolume(new_volume);
	return iDevSound.PlayVolume();
#else
	TSoundConfig sndConfig;
	iDevSound.Config(sndConfig); // read the default config
	sndConfig().iVolumeValue += aDelta;
	const TInt ret = iDevSound.SetConfig(sndConfig); // read the default config
	if(ret != KErrNone)
		{
		_ELOG_P2(_L8("AUDIO: Warning! could not set config due to %d\n"), ret);
		}
	iDevSound.Config(sndConfig); // read the default config again
	return sndConfig().iVolumeValue;
#endif // __ER6__
	}


// EOF - AUDIODRV.CPP
