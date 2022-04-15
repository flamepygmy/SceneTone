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

#if !defined(__SCENETONE_MIKMOD_H__)
#define      __SCENETONE_MIKMOD_H__

#include <e32base.h>
#include "scenetoneinterfaces.h"

#if defined(SCENETONE_MIKMOD_IMPLEMENTATION)
extern "C"
{
    #include <mikmod.h>
	MIKMODAPI extern struct MDRIVER drv_mem;
	extern    int   mikmod_mem_sample_mix_count;
	extern    void *mikmod_mem_sample_out_ptr;
}
#endif

class CScenetoneMikmod : public CBase, public MScenetoneSongProvider
{
  public:
    
    virtual TInt InitializeSong(const TDesC &aFileName, TInt aSampleRate, TInt aChannels, TInt &aSubSongs, TInt &aDefaultSong);
    virtual TInt StartSong( TInt aSubSong );
    virtual TInt SeekTo(TReal aSeconds);
    virtual TInt GetSamples(TDes8 &aBuffer);
    virtual void TerminateSong();

    ~CScenetoneMikmod();
    static CScenetoneMikmod* NewL();

  private:
    void ConstructL();

#if defined(SCENETONE_MIKMOD_IMPLEMENTATION)
    MODULE  		       *iModule;
#else
    TAny*                      foo;
#endif
	
};

CScenetoneMikmod *ScenetoneCreateMikmodProvider();

#endif
