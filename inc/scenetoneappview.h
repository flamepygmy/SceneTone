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

#ifndef __SCENETONEAPPVIEW_H__
#define __SCENETONEAPPVIEW_H__

// INCLUDES
#include <eikon.hrh>
#include <eikprogi.h>
#include <coecntrl.h>
#include <aknutils.h>

#include "scenetone3d.h"
#include "scenetoneappui.h"

// CLASS DECLARATION
class CScenetoneAppView : public CCoeControl
    {
    public: // New methods

        /**
        * NewL.
        * Two-phased constructor.
        * Create a CScenetoneAppView object, which will draw itself to aRect.
        * @param aRect The rectangle this view will be drawn to.
        * @return a pointer to the created instance of CScenetoneAppView.
        */
        static CScenetoneAppView* NewL( const TRect& aRect );

        /**
        * NewLC.
        * Two-phased constructor.
        * Create a CScenetoneAppView object, which will draw itself
        * to aRect.
        * @param aRect Rectangle this view will be drawn to.
        * @return A pointer to the created instance of CScenetoneAppView.
        */
        static CScenetoneAppView* NewLC( const TRect& aRect );

        /**
        * ~CScenetoneAppView
        * Virtual Destructor.
        */
        virtual ~CScenetoneAppView();
        
        void HandleResourceChange(TInt aType);

    public:  // Functions from base classes

        /**
        * From CCoeControl, Draw
        * Draw this CScenetoneAppView to the screen.
        * @param aRect the rectangle of this view that needs updating
        */
        void Draw( const TRect& aRect ) const;

        /**
        * From CoeControl, SizeChanged.
        * Called by framework when the view size is changed.
        */
        virtual void SizeChanged();

	virtual TInt CountComponentControls() const;
	virtual CCoeControl* ComponentControl( TInt aIndex ) const;

	void SetAppUi(CScenetoneAppUi *aUi);

#if defined(BUILD_5TH_EDITION_VERSION)
	void         HandlePointerEventL(const TPointerEvent& aPointerEvent);
#endif

    private: // Constructors

        /**
        * ConstructL
        * 2nd phase constructor.
        * Perform the second phase construction of a
        * CScenetoneAppView object.
        * @param aRect The rectangle this view will be drawn to.
        */
        void ConstructL(const TRect& aRect);

        /**
        * CScenetoneAppView.
        * C++ default constructor.
        */
        CScenetoneAppView();

    public:
	TBuf<128>	iPlayText;
	TBuf<128>       iPlayMinusOne;
	TBuf<128>       iPlayMinusTwo;
	TBuf<128>       iPlayPlusOne;
	TBuf<128>       iPlayPlusTwo;
	TBuf<128>       iPlayInfo;

	TBuf<128>       iBrowserSongs[16];
	TInt            iBrowserSongCount;
	TInt            iBrowserSongCurrentIndex;

	TRect           iswvolrect;
	TRect           ihwvolrect;
	TPoint          iswvoltext;
	TPoint          ihwvoltext;
        TRect           isongdragrect;

	TRect           ileftselrect;
	TRect           irightselrect;
	TRect           itopselrect;
	TRect           ibottomselrect;
	TRect           isongnamerect;

	    CScenetone3D *iScenetone3D;
	    CScenetoneAppUi *iAppUi;

	    CPeriodic        *iDragTimer;
	    TCallBack        iCallback;

	    TInt iTakingRecordTimes;
	    TBool iReceivingDrag;
	    TPoint iDragStart;
	    TInt   iDragInitialVolume;

	    TBool iHWVolDrag;
	    TBool iSWVolDrag;
	    TBool iSongDrag;

	    TInt iDragLastSong;
	    TInt iDragInitialSong;
    };

#endif // __SCENETONEAPPVIEW_H__

// End of File
