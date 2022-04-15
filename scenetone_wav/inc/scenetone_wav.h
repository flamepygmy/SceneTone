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

#if !defined(__SCENETONE_WAV_H__)
#define      __SCENETONE_WAV_H__

#include <e32base.h>
#include "scenetoneinterfaces.h"

class CScenetoneWav : public CBase, public MScenetoneFileWriter
{
  public:
    TInt GetSupportedRates(const TInt *&aSampleRates, const TInt *&aBitRates);
    TInt Start(const TDesC &aOutputFileName, TInt aSampleRate, TInt aChannels, TInt aSamples, TInt aBitRate, TInt (*aCallBack)(TAny *aOutput, TInt aBytes) );

    ~CScenetoneWav();
    static CScenetoneWav *NewL();

   private:
   	
    void   ConstructL();
};

CScenetoneWav *ScenetoneCreateWavWriter();

#endif
