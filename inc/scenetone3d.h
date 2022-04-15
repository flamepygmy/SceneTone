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

#ifndef __SCENETONE3D_H__
#define __SCENETONE3D_H__

#include <e32base.h>
#include <w32std.h>
#include <gdi.h>
#include "scenetoneinterfaces.h"

#include <GLES/egl.h>

class CScenetone3D : public CBase, public MScenetoneVisualizer
{
    public:
		virtual void NewSamples(TUint8 *aNewSamples, TInt aBytes, TBool aIsStereo);

        void Construct(RWindow aWin);
        ~CScenetone3D();

		void Start();
		void Stop();
		void UpdateViewport(TInt aWidth, TInt aHeight);
		void initGLES();
		void DoMorphs();
		
    private:
        static TInt DrawCallback( TAny* aInstance );
        TBool iGLInitialized;

        CPeriodic*  iPeriodic;  

        EGLDisplay  iEglDisplay;
        EGLSurface  iEglSurface;
        EGLContext  iEglContext;

        RWindow         iWin;
        TInt            iFrame;
		TInt			iTriangleCount;
		TInt			iSampleCounter;
		TInt			iMorphOff;
		float			iModelScale;
		
		short		  *iVertices;
		short 		  *iLiveVertices;
		unsigned char *iColors;
		int			  *iTriangleCoeffs;
};

#endif /* __SCENETONE3D_H__ */
