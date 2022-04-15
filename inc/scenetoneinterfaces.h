//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006  Jani Vaarala <flame@pygmyprojects.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  --------------------------------------------------------------------------

#if !defined(__SCENETONEINTERFACES_H__)
#define      __SCENETONEINTERFACES_H__

#include <e32std.h>
#include <e32base.h>
#include <e32cmn.h>

class MScenetoneSongProvider
{
  public:
    virtual TInt InitializeSong(const TDesC &aFileName, TInt aSampleRate, TInt aChannels, TInt &aSubSongs, TInt &aDefaultSong) = 0;
    virtual TInt StartSong( TInt aSubSong ) = 0;
    virtual TInt SeekTo(TReal aSeconds) = 0;
    virtual TInt GetSamples(TDes8 &aBuffer) = 0;
    virtual void TerminateSong() = 0;

    virtual ~MScenetoneSongProvider() { };
};

class MScenetoneVisualizer
{
  public:
  	virtual void NewSamples(TUint8 *samples, TInt aBytes, TBool aStereo) = 0;

	virtual ~MScenetoneVisualizer() { };
};

class MScenetoneFileWriter
{
  public:

    // assumption: mono/stereo is supported, samplerates and bitrates listed here should be supported all (all samplerates * all bitrates combinations)
	// Callback return value: bytes written. If return value < aBytes, there is no more input data
    virtual TInt GetSupportedRates(const TInt *&aSampleRates, const TInt *&aBitRates) = 0;

    // After start, processing goes on through callback until everything is done
    virtual TInt Start(const TDesC &aOutputFileName, TInt aSampleRate, TInt aChannels, TInt aSamples, TInt aBitRate, TInt (*aCallBack)(TAny *aOutput, TInt aBytes) ) = 0;

    virtual ~MScenetoneFileWriter() { };
};

#endif
