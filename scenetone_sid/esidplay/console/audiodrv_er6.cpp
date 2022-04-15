/**
 * @file audiodrv_er6.cpp Implements the class audioDriver, a thin wrapper around the
 * architecture specified sound driver. This file is based on the Linux version of 'audiodrv.cpp' by Michael Schwendt
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#include "audiodrv_er6.h"
#include "alaw.h"
#include "elog.h"

// does not work properly
#if defined(__CRYSTAL__) && defined(__WINS__)
#undef HAVE_AUDIO_DRIVER
#else
#define HAVE_AUDIO_DRIVER
#endif

const TInt KFileOpenWritable = EFileWrite | EFileShareAny | EFileStream;

struct wav_hdr                  // little endian
	{
	char main_chunk[4];         // 'RIFF'
	TUint32 length;              // filelength
	char chunk_type[4];         // 'WAVE'

	char sub_chunk[4];          // 'fmt '
	TUint32 clength;             // length of sub_chunk, always 16 bytes
	TUint16 format;              // currently always = 1 = PCM-Code
	TUint16 modus;               // 1 = mono, 2 = stereo
	TUint32 samplefreq;          // sample-frequency
	TUint32 bytespersec;         // frequency * bytespersmpl
	TUint16 bytespersmpl;        // bytes per sample; 1 = 8-bit, 2 = 16-bit
	TUint16 bitspersmpl;
	char data_chunk[4];         // keyword, begin of data chunk; = 'data'
	TUint32 data_length;         // length of data
	};

const wav_hdr my_wav_hdr =
{
	{'R','I','F','F'}, 0 /* file length */, {'W','A','V','E'},
	{'f','m','t',' '}, 16, 1, 1, SAMPLE_FREQ, SAMPLE_FREQ*2, 2, 16,
	{'d','a','t','a'}, 0 /* data length */
};


//
// implementation of audioDriver
//


audioDriver::audioDriver()
/**
 * C'tor
 */
	:audioHd(-1)
	,frequency(0)
	,encoding(0)
	,precision(0)
	,channels(0)
	,iIsReady(EFalse)
	,iBlocksInQueue(0)
	,iPrefilled(EFalse)
	,iIsWavDumping(0)
	{
	CTOR(audioDriver);
	}


void audioDriver::ConstructL()
/**
 * Reset everything, create and open audio stream
 */
	{
	errorString = "None";

	iSettings.iCaps = 0;
	iSettings.iMaxVolume = 0;

#ifdef HAVE_AUDIO_DRIVER
	iMdaAudio = CMdaAudioOutputStream::NewL(*this);
	iMdaAudio->Open(&iSettings);
#endif
	}


audioDriver::~audioDriver()
/**
 * D'tor
 */
	{
	DTOR(audioDriver);
	if(iMdaAudio)
		{
		iMdaAudio->Stop();
		delete iMdaAudio;
		iMdaAudio = NULL;
		}
	}


bool audioDriver::IsThere()
/**
 * Check device availability and write permissions.
 *
 * @return bool true if audio device is available
 */
	{
	return true;
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
	audioHd	= KErrNone; // means OK and opened

	// Transfer input parameters to this object.
	// May later be replaced with driver defaults.
	frequency = inFreq;
	precision = inPrecision;
	channels = inChannels;
	fragments = inFragments;
	fragSizeBase = inFragBase;

	// Set sample precision and type of encoding. (only 8 bit for EPOC)
	if (precision != SIDEMU_16BIT )
		{
		errorString = "AUDIO: Could not set sample size.(only 16 bit for ER6)";
		return false;
		}

	encoding = SIDEMU_SIGNED_PCM; // SIDEMU_UNSIGNED_PCM

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

	blockSize = KDefaultBufSize;
	fragments = 2; //TODO: find a value for this

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
		audioHd = (-1);
		}
	}


void audioDriver::Play(ubyte* pBuffer, int bufferSize)
/**
 * play a buffer with samples.
 *
 * @param pBuffer pointer to the buffer to be player
 * @param bufferSize number of bytes in the buffer
 */
	{
	if (audioHd != (-1))
		{
		/*
		 * WARNING! This descriptor *CAN NOT* be on the stack!!!
		 * keep it persisent. (argh)
		 */
		iExtraBuf.Copy(pBuffer, bufferSize);

// TODO just for testing
#ifdef HAVE_AUDIO_DRIVER
		TRAPD(ret, iMdaAudio->WriteL(iExtraBuf));
		if(ret)
			{
			ELOG2(_L8("WARNING! WriteL left with %d \n"), ret);
			}
#else
		User::After(200000); // simulate delay
#endif

		if(IsWavDumping())
			DoWavDump(iExtraBuf);
		}
	}


void audioDriver::StopStream(void)
/**
 * Close the audio stream
 */
	{
	if(iMdaAudio)
		{
		if(iIsReady)
			iMdaAudio->Stop();
		ResetStream();
		}
	}


void audioDriver::ResetStream(void)
/**
 * Reset the audio stream
 */
	{
	iBlocksInQueue = 0;
	iPrefilled = EFalse;
	}


TInt audioDriver::VolumeDelta(TInt aDelta)
/**
 * increase or decrease the volume by aDelta, and return the new volume
 */
	{
	iVolume += aDelta;
	if(iVolume < 0)
		iVolume = 0;
	else if(iVolume >= KVolumeSteps)
		iVolume = KVolumeSteps - 1;

	if(iMdaAudio && iIsReady)
		iMdaAudio->SetVolume(iVolume * iMdaAudio->MaxVolume()/KVolumeSteps );

	return iVolume;
	}


void audioDriver::MaoscOpenComplete(TInt aError)
/**
 * from MMdaAudioOutputStreamCallback
 *
 * called by Media Surfer when it is initialised (or not)
 */
	{
	ELOG2(_L8("MaoscOpenComplete [aError=%d] \n"), aError);

#if (SAMPLE_FREQ == 44100)
#define EPOC_SAMPLE_RATE TMdaAudioDataSettings::ESampleRate44100Hz
#elif (SAMPLE_FREQ == 8000)
#define EPOC_SAMPLE_RATE TMdaAudioDataSettings::ESampleRate8000Hz
#else
#error unsupported sample frequency
#endif

	// kick it
	if(aError == KErrNone)
		{
		// init audio again
		iMdaAudio->SetAudioPropertiesL(EPOC_SAMPLE_RATE,
			TMdaAudioDataSettings::EChannelsMono);
		iMdaAudio->SetPriority(EPriorityNormal, EMdaPriorityPreferenceNone);

		// update internal volume
		iVolume = iMdaAudio->MaxVolume() / 2 / KVolumeSteps;

		iIsReady = ETrue;
		}
	}


void audioDriver::MaoscBufferCopied(TInt aError, const TDesC8& /*aBuffer*/)
/**
 * called by Media Surfer when finished copying
 *
 * Note: In reality, this function is called approx. 1 millisecond after the
 * last block was played, hence we have to generate buffer N+1 while buffer N
 * is playing.
 */
	{
	if(aError)
		{
		ELOG2(_L8("WARNING! MaoscBufferCopied [aError=%d] \n"), aError);
		}

	// check if stream was closed
	if(aError == KErrCancel)
		{
		ResetStream();
		}

	iBlocksInQueue = 0;
	}


void audioDriver::MaoscPlayComplete(TInt aError)
/**
 * called by Media Surfer when stream was played
 */
	{
	if(aError)
		{
		ELOG2(_L8("MaoscPlayComplete [aError=%d] \n"), aError);
		}

	// check if stream was closed
	if(aError == KErrCancel)
		{
		ResetStream();
		}
	}


TInt audioDriver::StartWavDump(const TDesC& aSidTune)
/**
 * Start dumping to WAV file
 */
	{
	ELOG(TBuf8<128> buf8;)
	ELOG(buf8.Copy(aSidTune);)
	ELOG2(_L8("Starting WAV dump of '%S.wav'\n"), &buf8);

	if(iIsWavDumping)
		{
		ELOG1(_L8("already dumping wav\n"));
		return KErrAlreadyExists;
		}

	TInt ret = iFs.Connect();
	if( (ret != KErrNone) && (ret != KErrAlreadyExists) )
		return ret;

	iWavDump.Copy(aSidTune);
	iWavDump.Append(_L(".wav"));

	// just in case
	(void)iFs.Delete(iWavDump);

	RFile file;
	ret = file.Open(iFs, iWavDump, KFileOpenWritable);
	if(ret == KErrNotFound)
		ret = file.Replace(iFs, iWavDump, KFileOpenWritable);
	if(ret == KErrNone)
		{
		// write WAV header
		TPtrC8 ptr((TUint8*)&my_wav_hdr, sizeof(wav_hdr));
		ret = file.Write(ptr);
		if(ret)
			{
			file.Close();
			return ret;
			}
		}
	else
		return ret;

	file.Close();

	// alls well
	iIsWavDumping = ETrue;
	return KErrNone;
	}


TInt audioDriver::StopWavDump()
/**
 * Stop dumping to WAV file
 */
	{
	ELOG(TBuf8<128> buf8;)
	ELOG(buf8.Copy(iWavDump);)
	ELOG2(_L8("Stopping WAV dump of '%S'\n"), &buf8);

	RFile file;
	TInt ret = file.Open(iFs, iWavDump, KFileOpenWritable);
	if(ret)
		{
		ELOG2(_L8("Serious problem, was supposed to open file [ret=%d]\n"), ret);
		iIsWavDumping = EFalse;
		return ret;
		}

	// update WAV header
	TInt size = 0;
	ret = file.Size(size);
	if(ret)
		{
		ELOG2(_L8("Wavdump: could not get size [ret=%d]\n"), ret);
		iIsWavDumping = EFalse;
		return ret;
		}

	/*
	 * I did not find any better way of doing this, please
	 * improve this code if any better solutions exist.
	 */
	const TUint32 file_length = size - 8;
	const TUint32 data_length = size - sizeof(wav_hdr);

	TBuf8<4> file_len;
	file_len.SetLength(4);
	file_len[0] = (TUint8)((file_length >> 0) & 0xff);
	file_len[1] = (TUint8)((file_length >> 8) & 0xff);
	file_len[2] = (TUint8)((file_length >> 16) & 0xff);
	file_len[3] = (TUint8)((file_length >> 24) & 0xff);

	TBuf8<4> data_len;
	data_len.SetLength(4);
	data_len[0] = (TUint8)((data_length >> 0) & 0xff);
	data_len[1] = (TUint8)((data_length >> 8) & 0xff);
	data_len[2] = (TUint8)((data_length >> 16) & 0xff);
	data_len[3] = (TUint8)((data_length >> 24) & 0xff);

	(void)file.Write(4, file_len);
	(void)file.Write(40, data_len);

	iFs.Close();
	iIsWavDumping = EFalse;
	return size;
	}


TBool audioDriver::IsWavDumping()
/**
 * Return status of WAV dumping
 */
	{
	return iIsWavDumping;
	}


void audioDriver::DoWavDump(const TDesC8& aBuffer)
/**
 * Dump one PCM fragment to the WAV file
 */
	{
	RFile file;
	TInt ret = file.Open(iFs, iWavDump, KFileOpenWritable);
	if(ret)
		{
		ELOG2(_L8("Serious problem, was supposed to open wav file [ret=%d]\n"), ret);
		return;
		}

	// append the stream
	TInt pos;
	ret = file.Seek(ESeekEnd, pos);
	if (ret==KErrNone)
		{
		(void)file.Write(aBuffer);
		}
	else
		{
		ELOG2(_L8("Wavdump: could not seek due to %d\n"), ret);
		}

	file.Close();
	}


// EOF - AUDIODRV_ER6.CPP
