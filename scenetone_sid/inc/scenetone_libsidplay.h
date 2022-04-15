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

#if !defined(__SCENETONE_LIBSIDPLAY_H__)
#define      __SCENETONE_LIBSIDPLAY_H__

#if defined(SCENETONE_LIBSIDPLAY_IMPLEMENTATION)
#  include "player.h"
#endif

#include <e32base.h>
#include "scenetoneinterfaces.h"

class CScenetoneSidplay : public CBase, public MScenetoneSongProvider
{
  public:
    TInt InitializeSong(const TDesC &aFileName, TInt aSampleRate, TInt aChannels, TInt &aSubSongs, TInt &aDefaultSong);
    TInt StartSong( TInt aSubSong );
    TInt SeekTo(TReal aSeconds);
    TInt GetSamples(TDes8 &aBuffer);
    void TerminateSong();

    ~CScenetoneSidplay();
	static CScenetoneSidplay *NewL();

   private:
   	
    void   ConstructL();

#if defined(SCENETONE_LIBSIDPLAY_IMPLEMENTATION)
    emuEngine *iSIDEngine;
    sidTune   *iSIDTune;
#else
    void      *reserved1;
    void      *reserved2;
#endif
};

CScenetoneSidplay *ScenetoneCreateSIDProvider();

#endif
