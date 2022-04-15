// sidplayer.cpp
//
// Copyright (c) 2001-2002 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//

/**
 * @file sidplayer.cpp
 *
 * Implements the container for SID emulator and tunes
 */

#include "sidplayer.h"

enum
{
	ERR_NOT_ENOUGH_MEMORY,
	ERR_SYNTAX,
	ERR_ENDIANESS
};
const TInt EXIT_ERROR_STATUS = (-1);

//
// forward declarations
//
GLREF_C bool sidEmuInitializeSong(emuEngine& thisEmuEngine,
				  sidTune& thisTune,
				  uword songNumber);


TInt SidPlayerThread(TAny* aPlayer)
/**
 * this is the Sid player idle thread.
 */
	{
	CSidPlayer* play = (CSidPlayer*)aPlayer;

	if(play->IsPaused())
		return ETrue;

	play->Thread();
	return ETrue;
	}


//
// implementation of CSidPlayer
//


CSidPlayer* CSidPlayer::NewL(const TDesC& aSong)
	{
	CSidPlayer* p = new (ELeave) CSidPlayer(aSong);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop(p);
	return p;
	}


CSidPlayer::~CSidPlayer()
/**
 * D'tor
 */
	{
	DTOR(CSidPlayer);

	delete iEmuEngine;
	delete iAudio;
	delete iTune;
	delete[] iBuffer;

	if(iIdlePlay)
		delete iIdlePlay;
	}


void CSidPlayer::ConstructL()
/**
 * safe constructor
 */
	{
	InitL();
	}


CSidPlayer::CSidPlayer(const TDesC& aSong)
/**
 * C'tor
 */
	:iIdlePlay(NULL)
	,iIsReady(EFalse)
	{
	CTOR(CSidPlayer);
	iSongName.Copy(aSong);
	}


void CSidPlayer::InitL()
/**
 * initialise everything, but do not start playing (yet)
 */
	{
	ELOG1(_L8("CSidPlayer::InitL\n"));
	TInt ret;

	if(iIsReady)
		{
		ELOG1(_L8("Hmm. already inited\n"));
		return;
		}

	// ======================================================================
	// INITIALIZE THE EMULATOR ENGINE
	// ======================================================================

	iEmuEngine = new (ELeave) emuEngine();

	// Initialize the SID-Emulator Engine to defaults.
	if ( !iEmuEngine->verifyEndianess() )
		{
		User::Leave(ERR_ENDIANESS);
		}

	// Get the default configuration.
	struct emuConfig myEmuConfig;
	iEmuEngine->getConfig(myEmuConfig);

	uword selectedSong = 0;

#ifdef __ER6__
	myEmuConfig.frequency = SAMPLE_FREQ;
	myEmuConfig.channels = SIDEMU_MONO;
	myEmuConfig.bitsPerSample = SIDEMU_16BIT;
#else
	myEmuConfig.frequency = KAlawSamplesPerSecond;
	myEmuConfig.channels = SIDEMU_MONO;
	myEmuConfig.bitsPerSample = SIDEMU_8BIT;
#endif
	uword fragments = 16;
	uword fragSizeBase = 12;
	int forceBufSize = 0;  // get it from argument line

	// ======================================================================
	// VALIDATE SID EMULATOR SETTINGS
	// ======================================================================

	if ((myEmuConfig.autoPanning!=SIDEMU_NONE) && (myEmuConfig.channels==SIDEMU_MONO))
		{
		myEmuConfig.channels = SIDEMU_STEREO;  // sane
		}
	if ((myEmuConfig.autoPanning!=SIDEMU_NONE) && (myEmuConfig.volumeControl==SIDEMU_NONE))
		{
		myEmuConfig.volumeControl = SIDEMU_FULLPANNING;  // working
		}

	// ======================================================================
	// INSTANTIATE A SIDTUNE OBJECT
	// ======================================================================

	// warning! 16-bit descriptor truncated to 8-bit
	TBuf8<256> buf8;
	buf8.Copy(iSongName);
	iTune = new (ELeave) sidTune( (const char*)buf8.PtrZ() );
	struct sidTuneInfo mySidInfo;
	iTune->getInfo( mySidInfo );
	if ( !iTune )
		{
		User::Leave(EXIT_ERROR_STATUS);
		}

	// ======================================================================
	// CONFIGURE THE AUDIO DRIVER
	// ======================================================================

#if defined (__WINS__)
#define PDD_NAME _L("ESDRV")
#define LDD_NAME _L("ESOUND")

	ret = User::LoadPhysicalDevice(PDD_NAME);
	if (ret!=KErrNone && ret!=KErrAlreadyExists)
		User::LeaveIfError(ret);

	ret = User::LoadLogicalDevice(LDD_NAME);
	if (ret!=KErrNone && ret!=KErrAlreadyExists)
		User::LeaveIfError(ret);
#endif

	// Instantiate the audio driver. The capabilities of the audio driver
	// can override the settings of the SID emulator.
	iAudio = new (ELeave) audioDriver();

	iAudio->ConstructL();

	if ( !iAudio->IsThere() )
		{
		User::Leave(EXIT_ERROR_STATUS);
		}

	if ( !iAudio->Open(myEmuConfig.frequency, myEmuConfig.bitsPerSample,
					   myEmuConfig.channels, fragments, fragSizeBase))
		{
		ELOG1(_L8("Could not open audio\n"));
		User::Leave(KErrNotReady);
		}


	// ======================================================================
	// CONFIGURE THE EMULATOR ENGINE
	// ======================================================================

	// Configure the SID emulator according to the audio driver settings.
	myEmuConfig.frequency = (uword)iAudio->GetFrequency();
	myEmuConfig.bitsPerSample = iAudio->GetSamplePrecision();
	myEmuConfig.sampleFormat = iAudio->GetSampleEncoding();
	ret = iEmuEngine->setConfig( myEmuConfig );
	if(!ret)
		{
		ELOG1(_L8("Could not SetConfig on emu engine\n"));
		User::Leave(KErrNotReady);
		}

	// ======================================================================
	// INITIALIZE THE EMULATOR ENGINE TO PREPARE PLAYING A SIDTUNE
	// ======================================================================

	if ( !sidEmuInitializeSong(*iEmuEngine, *iTune, selectedSong) )
		{
		ELOG1(_L8("could not initialise emu engine\n"));
		User::Leave(KErrNotReady);
		}


	// ======================================================================
	// KEEP UP A CONTINUOUS OUTPUT SAMPLE STREAM
	// ======================================================================

	iBufSize = iAudio->GetBlockSize();
	if (forceBufSize != 0)
		{
		iBufSize = forceBufSize;
		}

	if(iBufSize <= 0 )
		{
		User::Leave(EXIT_ERROR_STATUS);
		}

	iBuffer = new (ELeave) ubyte[iBufSize];

	//
	// OK - everything is now ready for ::Play();
	//
	iIsReady = ETrue;
	}


void CSidPlayer::Play()
/**
 * start playing using the CIdle class
 */
	{
	ELOG1(_L8("CSidPlayer::Play\n"));

	if(!iIsReady)
		{
		ELOG1(_L8("Oops. Not ready...\n"));
		return;
		}

	// if already playing, just return
	if(iIdlePlay)
		return;

	// create a new CIdle object and start it
	iIdlePlay = CIdle::NewL(CActive::EPriorityIdle);
	iIdlePlay->Start(TCallBack(SidPlayerThread, this));
	}


void CSidPlayer::Stop()
/**
 * stop playing
 */
	{
	ELOG1(_L8("CSidPlayer::Stop\n"));

	if(iAudio)
		iAudio->StopStream();

	if(iIdlePlay)
		{
		delete iIdlePlay;
		iIdlePlay = NULL;
		}
	sidEmuInitializeSong(*iEmuEngine, *iTune, 0);
	}


void CSidPlayer::Pause()
/**
 * pause the player
 */
	{
	ELOG1(_L8("CSidPlayer::Pause\n"));

//	if(iAudio)
//		iAudio->StopStream(); ?

	if(iIdlePlay)
		{
		delete iIdlePlay;
		iIdlePlay = NULL;
		}
	else
		Play();
	}


TBool CSidPlayer::IsPaused()
/**
 * return ETrue if paused
 */
	{
	return EFalse; /// @todo implement
	}


void CSidPlayer::Thread()
/**
 * wow. here is the actual core loop of the system;
 * fill the buffer, then play it. repeat 1000 times.
 */
	{
#if defined __ER6__ && !defined(__WINS__)
	if(!iAudio->iIsReady)
		return;

	if(!iAudio->iPrefilled)
		{
		iEmuEngine->FillBuffer(*iTune, iBuffer, iBufSize);
		iAudio->iPrefilled = ETrue;
		}

	if(iAudio->iPrefilled && iAudio->iBlocksInQueue == 0)
		{
		iAudio->Play((ubyte*)iBuffer, iBufSize);
		iAudio->iBlocksInQueue = 1;
		iAudio->iPrefilled = EFalse;
		}
#else
	iEmuEngine->FillBuffer(*iTune, iBuffer, iBufSize);
	iAudio->Play(iBuffer, iBufSize);
#endif
	}


TInt CSidPlayer::VolumeDelta(TInt aDelta)
/**
 * increase/decrease volume and return the new volume
 */
	{
	return iAudio->VolumeDelta(aDelta);
	}


TInt CSidPlayer::SongDelta(TInt aDelta)
/**
 * skip song up/down and return the new song numer
 */
	{
	struct sidTuneInfo mySidInfo;
	iTune->getInfo( mySidInfo );

	if(mySidInfo.songs == 1)
		return mySidInfo.currentSong;

	if(iAudio)
		iAudio->StopStream();

	TInt song = (mySidInfo.currentSong + aDelta) % mySidInfo.songs;
	if(song == 0)
			song = mySidInfo.songs;

	if ( !sidEmuInitializeSong(*iEmuEngine, *iTune, (uword)song) )
		{
		ELOG1(_L8("could not initialise emu engine\n"));
		}

	Play();

	iTune->getInfo( mySidInfo );
	return mySidInfo.currentSong;
	}


TInt CSidPlayer::SongSelect(TInt& aSong)
/**
 * select a new song to play
 */
	{
	if(!sidEmuInitializeSong(*iEmuEngine, *iTune, (uword)aSong) )
		return KErrNotReady;

	if(iAudio)
		iAudio->StopStream();

	struct sidTuneInfo mySidInfo;
	iTune->getInfo( mySidInfo );
	aSong = mySidInfo.currentSong;

	Play();

	return KErrNone;
	}


TInt CSidPlayer::NewTune(const TDesC& aTune)
/**
 * open a new SID tune, delete the old one
 */
	{
	delete iTune;
	iSongName.Copy(aTune);

	// warning! 16-bit descriptor truncated to 8-bit
	TBuf8<256> buf8;
	buf8.Copy(iSongName);
	iTune = new (ELeave) sidTune( (const char*)buf8.PtrZ() );

	if(sidEmuInitializeSong(*iEmuEngine, *iTune, 0 /* default song */ ) )
		return KErrNone;
	else
		return KErrNotReady;
	}
