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

#define SCENETONE_MIKMOD_IMPLEMENTATION

#include <e32std.h>
#include "scenetone_mikmod.h"

#include <e32std.h>
#include <f32file.h>
#include <utf.h>

TInt CScenetoneMikmod::InitializeSong(const TDesC &aFileName, TInt aSampleRate, TInt aChannels, TInt &aSubSongs, TInt &aDefaultSong)
{ 
    TInt err;
	
    // Convert filename to suite LIBC
    TBuf8<KMaxFileName+1> fname8;
    CnvUtfConverter::ConvertFromUnicodeToUtf8(fname8, aFileName);
    fname8.Append(TChar('\0'));

	// if there is one already initialized -> kill it before starting a new one
	TerminateSong();		

    mikmod_mem_sample_mix_count = 0;
    mikmod_mem_sample_out_ptr   = NULL;

    // TODO:: if already initialized -> error..

    md_device  = 1;
    md_mode    = (DMODE_16BITS|DMODE_SOFT_MUSIC|DMODE_INTERP);
    md_mixfreq = aSampleRate; 

    if( aChannels == 2 ) 	md_mode |= DMODE_STEREO;

    err = MikMod_Reset("");
    if(err)
    {	
		return KErrGeneral;
	}
	
    /* load module */
    iModule = Player_Load((char*)fname8.Ptr(), 64, 0);
    if (iModule)
    {
		/* start module */
		Player_Start(iModule);
    }
    else
    {
    	return KErrGeneral;
    }

    aSubSongs    = 1;
    aDefaultSong = 0;

    return KErrNone;
}

TInt CScenetoneMikmod::StartSong(TInt aSubSong)
{
    return KErrNone;
}


TInt CScenetoneMikmod::SeekTo(TReal aSeconds)
{
    return KErrNotSupported;
}

TInt CScenetoneMikmod::GetSamples(TDes8 &aBuffer)
{
    mikmod_mem_sample_mix_count = aBuffer.Length();
    mikmod_mem_sample_out_ptr   = (void*)aBuffer.Ptr();
    MikMod_Update();
    return KErrNone;
}

void CScenetoneMikmod::TerminateSong()
{
	if(iModule)
	{
		Player_Stop();
		Player_Free(iModule);
		iModule = NULL;
	}
}

CScenetoneMikmod *CScenetoneMikmod::NewL()
{
	CScenetoneMikmod *p = new (ELeave) CScenetoneMikmod;
	p->ConstructL();
	return p;
}

EXPORT_C CScenetoneMikmod *ScenetoneCreateMikmodProvider()
{
	CScenetoneMikmod *p = CScenetoneMikmod::NewL();
	return p;
}

void CScenetoneMikmod::ConstructL()
{
    mikmod_mem_sample_mix_count = 0;
    mikmod_mem_sample_out_ptr   = NULL;

    MikMod_RegisterDriver(&drv_mem);          /* 1: memory driver */
    MikMod_RegisterAllLoaders();

    md_device  = 1;
    md_mode    = (DMODE_16BITS|DMODE_SOFT_MUSIC|DMODE_INTERP|DMODE_STEREO);
    md_mixfreq = 44100; 

    if (MikMod_Init(""))
    {
		User::Panic(_L("debug..."),7);
    }
}

CScenetoneMikmod::~CScenetoneMikmod()
{
	TerminateSong();
	MikMod_Exit();
}
