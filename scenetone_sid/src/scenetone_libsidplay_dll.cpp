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

#define SCENETONE_LIBSIDPLAY_IMPLEMENTATION

#include <e32std.h>
#include "scenetone_libsidplay.h"
#include "emucfg.h"

#include <f32file.h>
#include <utf.h>

GLREF_C bool sidEmuInitializeSong(emuEngine & thisEmuEngine,
								  sidTune & thisTune,
								  uword songNumber);



TInt CScenetoneSidplay::InitializeSong(const TDesC &aFileName, TInt aSampleRate, TInt aChannels, TInt &aSubSongs, TInt &aDefaultSong)
{ 
    // Convert filename to suite LIBC
    TBuf8<KMaxFileName+1> fname8;
    CnvUtfConverter::ConvertFromUnicodeToUtf8(fname8, aFileName);
    fname8.Append(TChar('\0'));

    // TODO:: if already initialized -> error..

	// Terminate old song, if it was active
	if(iSIDTune)		TerminateSong();

    // Get the default configuration.
    struct emuConfig myEmuConfig;
    iSIDEngine->getConfig(myEmuConfig);
    myEmuConfig.frequency = aSampleRate;
    
    // Fill in the configuration data
	if(aChannels == 1)    myEmuConfig.channels = SIDEMU_MONO;
	else				  myEmuConfig.channels = SIDEMU_STEREO;
    myEmuConfig.bitsPerSample = SIDEMU_16BIT;
    myEmuConfig.sampleFormat  = SIDEMU_SIGNED_PCM;

    iSIDTune = new (ELeave) sidTune( (const char*)fname8.Ptr() );
    iSIDEngine->setConfig( myEmuConfig ); // todo: error codes..

    struct sidTuneInfo mySidInfo;
    iSIDTune->getInfo( mySidInfo );
    aSubSongs = mySidInfo.songs;
    aDefaultSong = mySidInfo.startSong;
    
	return KErrNone;
}

TInt CScenetoneSidplay::StartSong(TInt aSubSong)
{
    if( !sidEmuInitializeSong(*iSIDEngine, *iSIDTune, aSubSong) ) { return KErrGeneral; }
	return KErrNone;
}


TInt CScenetoneSidplay::SeekTo(TReal aSeconds)
{
    return KErrNotSupported;
}

TInt CScenetoneSidplay::GetSamples(TDes8 &aBuffer)
{
    iSIDEngine->FillBuffer(*iSIDTune, (void*)aBuffer.Ptr(), aBuffer.Length());
	return KErrNone;
}

void CScenetoneSidplay::TerminateSong()
{
	//sidEmuInitializeSong(*iSIDEngine, *iSIDTune, 0);
	delete iSIDTune;
	iSIDTune = NULL;
}

CScenetoneSidplay *CScenetoneSidplay::NewL()
{
	CScenetoneSidplay *p = new (ELeave) CScenetoneSidplay;
	p->ConstructL();
	return p;
}

EXPORT_C CScenetoneSidplay *ScenetoneCreateSIDProvider()
{
	CScenetoneSidplay *p = CScenetoneSidplay::NewL();
	return p;
}

void CScenetoneSidplay::ConstructL()
{
    iSIDEngine = new (ELeave) emuEngine();
    if(!iSIDEngine->verifyEndianess()) { User::Panic(_L("joo"),1); }
}

CScenetoneSidplay::~CScenetoneSidplay()
{
	if(iSIDTune)		TerminateSong();
	delete iSIDEngine;
}
