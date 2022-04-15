// sidplayer.h
//
// Copyright (c) 2001 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//

/**
 * @file sidplayer.h
 *
 * Declares the container for SID emulator and tunes
 */
#if !defined(SIDPLAYER_H__)
#define SIDPLAYER_H__

#include "player.h"
#include "myendian.h"
#if defined(__ER6__)
#include "audiodrv_er6.h"
#else
#include "audiodrv_epoc.h"
#endif


/**
 * contains one SID tune, and controls the playback of it
 */
class CSidPlayer : public CBase
	{
public:
	static CSidPlayer* NewL(const TDesC& aSong);
	virtual ~CSidPlayer();
	//
	void InitL();
	void Play();
	void Stop();
	void Pause();
	TBool IsPaused();
	void Thread();
	sidTune& CurrentSidTune() const {return *iTune;}
	TInt VolumeDelta(TInt aDelta);
	TInt SongDelta(TInt aDelta);
	TInt SongSelect(TInt& aSong);
	TInt NewTune(const TDesC& aTune);
protected:
	void ConstructL();
	CSidPlayer(const TDesC& aSong);
public:
	emuEngine*   iEmuEngine; ///< pointer to the emulator engine
	CIdle*       iIdlePlay;  ///< pointer to the CIdle playback object
	TBuf<128>    iSongName;  ///< name of the song
	audioDriver* iAudio;     ///< pointer to the audio driver
private:
	sidTune*     iTune;      ///< pointer to the SID tune
	ubyte*       iBuffer;    ///< buffer used for sound
	TInt         iBufSize;   ///< size of the sound buffer
	TBool        iIsReady;   ///< ETrue if inited and ready to play
	};


#endif // SIDPLAYER_H__
