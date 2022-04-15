//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006-2008  Jani Vaarala <flame@pygmyprojects.com>
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


// INCLUDE FILES
#include <coemain.h>
#include "scenetoneappview.h"
#include "scenetone3d.h"

#include <gulbordr.h>
#include <gulutil.h>

#include <eikenv.h>

#include <akndef.h>
#include <aknview.h>

#define SEL_DIAMOND_DELTA_X      6
#define SEL_DIAMOND_DELTA_Y      0


static TInt DragCallback(TAny *ptr)
{
  CScenetoneAppView *t = (CScenetoneAppView *)ptr;
  if(t->iSongDrag)
  {
    t->iAppUi->DirectJumpSong(t->iDragLastSong, EFalse);
  }

  // drag stopped!
  t->iSongDrag      = EFalse;
  t->iDragTimer->Deque();
  t->DrawNow();

  return 0;
}



// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CScenetoneAppView::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CScenetoneAppView* CScenetoneAppView::NewL( const TRect& aRect )
    {
    CScenetoneAppView* self = CScenetoneAppView::NewLC( aRect );
    CleanupStack::Pop( self );
    return self;
    }

// -----------------------------------------------------------------------------
// CScenetoneAppView::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CScenetoneAppView* CScenetoneAppView::NewLC( const TRect& aRect )
    {
    CScenetoneAppView* self = new ( ELeave ) CScenetoneAppView;
    CleanupStack::PushL( self );
    self->ConstructL( aRect );
    return self;
    }



// -----------------------------------------------------------------------------
// CScenetoneAppView::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//

_LIT(KSpacedOut, " ");

void CScenetoneAppView::ConstructL( const TRect& aRect )
{
    iCallback = TCallBack(DragCallback, (TAny*)this);

    iScenetone3D = new CScenetone3D;

    iPlayText.Format(KSpacedOut);
    iPlayMinusTwo.Format(KSpacedOut); 
    iPlayMinusOne.Format(KSpacedOut); 
    iPlayPlusTwo.Format(KSpacedOut); 
    iPlayPlusOne.Format(KSpacedOut); 
    iBrowserSongCount = 15;            // TODO: change on orientation change and resolution...
                                       // always ODD so that it is divisible by 2 + there is one "middle" one
    iBrowserSongCurrentIndex = iBrowserSongCount / 2;

    // Create a window for this application view
    CreateWindowL();

#if defined(BUILD_5TH_EDITION_VERSION)
    EnableDragEvents();
    
    int endeth     = 20 + iBrowserSongCount * 20 + 20;

    iswvolrect     = TRect(130,115+30+endeth-120,230,215+30+endeth-120);
    ihwvolrect     = TRect(250,115+30+endeth-120,350,215+30+endeth-120);
    iswvoltext     = TPoint(150,175+30+endeth-120);
    ihwvoltext     = TPoint(270,175+30+endeth-120);

    isongdragrect  = TRect(SEL_DIAMOND_DELTA_X+10+25-3-3,     SEL_DIAMOND_DELTA_Y+115+30-3-3+endeth-120,
			   SEL_DIAMOND_DELTA_X+10+25+50+3+3,  SEL_DIAMOND_DELTA_Y+215+30+3+3+endeth-120);

    ileftselrect   = TRect(0  +10, 25 +145+endeth-120, 50  +10,  75 +145+25+endeth-120);
    irightselrect  = TRect(50 +10, 25 +145+endeth-120, 100 +10,  75 +145+25+endeth-120);
    itopselrect    = TRect(25 +10, 0  +145+endeth-120, 75  +10,  25 +145+25+endeth-120);
    ibottomselrect = TRect(25 +10, 75 +145+endeth-120, 75  +10, 100 +145+25+endeth-120);

    isongnamerect  = TRect(10, 10, 350, 100+endeth-120);
#endif

    // Set the windows size
   	SetRect( aRect );
	
    // Activate the window, which makes it ready to be drawn
    ActivateL();

    iScenetone3D->Construct(Window());
	iScenetone3D->UpdateViewport( Size().iWidth, Size().iHeight );
}

// -----------------------------------------------------------------------------
// CScenetoneAppView::CScenetoneAppView()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CScenetoneAppView::CScenetoneAppView()
    {
    // No implementation required
    }


// -----------------------------------------------------------------------------
// CScenetoneAppView::~CScenetoneAppView()
// Destructor.
// -----------------------------------------------------------------------------
//
CScenetoneAppView::~CScenetoneAppView()
    {
    // No implementation required
    }


// -----------------------------------------------------------------------------
// CScenetoneAppView::Draw()
// Draws the display.
// -----------------------------------------------------------------------------
//
#if !defined(BUILD_5TH_EDITION_VERSION)
void CScenetoneAppView::Draw( const TRect& aRect ) const
    {
    // Get the standard graphics context
#if !defined(SCENETONE_INCLUDE_VISUALIZER)
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle( CGraphicsContext::ENullPen );
    gc.SetBrushColor( TRgb(120,120,120) );
    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

    // Clears the screen
    gc.Clear( aRect );
    
    gc.UseFont( CEikonEnv::Static()->DenseFont() );
	
    // draw the view of currently playing and 2 before and 2 after that (in the directory)
    gc.SetPenColor(KRgbBlue);
    gc.DrawText( iPlayMinusTwo, TPoint(10,20) );
    gc.DrawText( iPlayMinusOne, TPoint(10,35) );
    gc.DrawText( iPlayPlusOne,  TPoint(10,65) );
    gc.DrawText( iPlayPlusTwo,  TPoint(10,80) );

    gc.SetPenColor(KRgbGreen);
    gc.DrawText( iPlayInfo,     TPoint(10,130));
    
    gc.SetPenColor(KRgbWhite);
    gc.DrawText( iPlayText,  TPoint(10,50) );

    if(iTakingRecordTimes)
    {
      gc.SetPenColor(KRgbRed);
      gc.DrawText( _L("MARKER CAPTURE"), TPoint(10,105) );
    }
#endif
}

#else

void CScenetoneAppView::Draw( const TRect& aRect ) const
{
// TOUCH VERSION
#if !defined(SCENETONE_INCLUDE_VISUALIZER)
    CWindowGc& gc = SystemGc();

    gc.SetPenStyle( CGraphicsContext::ENullPen );
    gc.SetBrushColor( TRgb(120,120,120) );
    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

    // Clears the screen
    gc.Clear( aRect );
    
    gc.UseFont( CEikonEnv::Static()->DenseFont() );
	
    // draw the view of currently playing and 2 before and 2 after that (in the directory)
    gc.SetPenColor(KRgbBlue);

    int endeth = 120;

#if defined(BUILD_5TH_EDITION_VERSION)
    int i;
    int cnt = iBrowserSongCount;

    for(i=0;i<cnt;i++)
    {
      if(i == iBrowserSongCurrentIndex)
      {
	// draw with different color
	gc.SetPenColor(KRgbWhite);
	gc.DrawText( iBrowserSongs[i], TPoint(10,20+i*20));      
	gc.SetPenColor(KRgbBlue);
      }
      else
      {
	gc.DrawText( iBrowserSongs[i], TPoint(10,20+i*20));
      }
    }

    endeth = 20 + cnt * 20 + 20;

#else
    gc.DrawText( iPlayMinusTwo, TPoint(10,20) );
    gc.DrawText( iPlayMinusOne, TPoint(10,40) );
    gc.DrawText( iPlayPlusOne,  TPoint(10,80) );
    gc.DrawText( iPlayPlusTwo,  TPoint(10,100) );

    gc.SetPenColor(KRgbWhite);
    gc.DrawText( iPlayText,  TPoint(10,60) );

    endeth = 120;

#endif

    gc.SetPenColor(KRgbGreen);
    gc.DrawText( iPlayInfo,     TPoint(10,endeth + 7 ));
    

    gc.SetPenStyle( CGraphicsContext::ESolidPen );
    
    if(iTakingRecordTimes)
    {
      gc.SetPenColor(KRgbRed);
      gc.SetBrushColor(KRgbRed);
	
      gc.DrawRect(TRect(330,10,350,30));
    }

    gc.SetPenColor(KRgbWhite);

    // draw selection diamond (main song, subsong)
    gc.SetBrushColor(KRgbBlue);

    CArrayFix<TPoint> *points = new CArrayFixFlat<TPoint>(4);
    points->AppendL(TPoint(SEL_DIAMOND_DELTA_X+10-3-3,SEL_DIAMOND_DELTA_Y+165+30+endeth-120));
    points->AppendL(TPoint(SEL_DIAMOND_DELTA_X+60,SEL_DIAMOND_DELTA_Y+115+30-3-3+endeth-120));
    points->AppendL(TPoint(SEL_DIAMOND_DELTA_X+110+3+3,SEL_DIAMOND_DELTA_Y+165+30+endeth-120));
    points->AppendL(TPoint(SEL_DIAMOND_DELTA_X+60,SEL_DIAMOND_DELTA_Y+215+30+3+3+endeth-120));
    gc.DrawPolygon(points);

    gc.SetBrushColor( TRgb(120,120,120) );
    gc.SetPenColor( TRgb(120,120,120));

    if(iSongDrag) gc.SetBrushColor(TRgb(200,1260,120));

    gc.DrawRect(TRect(SEL_DIAMOND_DELTA_X+10+25-3-3,SEL_DIAMOND_DELTA_Y+115+25+30-3-3+endeth-120,
		      SEL_DIAMOND_DELTA_X+10+25+50+3+3,SEL_DIAMOND_DELTA_Y+115+25+50+30+3+3+endeth-120));

    gc.SetBrushColor( TRgb(120,120,120) );
    gc.SetPenColor( KRgbWhite );

    // todo: optimize unnecessary overlapped rendering away from above...
    //if(iSongDrag)       gc.SetBrushColor( TRgb(200,160,120) );
    //else                gc.SetBrushColor( TRgb(120,120,120) );
    //    gc.DrawEllipse(isongdragrect);
   
    if(iSWVolDrag)      gc.SetBrushColor( TRgb(200,160,120) );
    else                gc.SetBrushColor( TRgb(120,120,120) );
    gc.DrawEllipse(iswvolrect);

    if(iHWVolDrag)      gc.SetBrushColor( TRgb(200,160,120) );
    else                gc.SetBrushColor( TRgb(120,120,120) );
    gc.DrawEllipse(ihwvolrect);

    gc.DrawText(_L("SWVol"), iswvoltext);
    gc.DrawText(_L("HWVol"), ihwvoltext);
#endif
}
#endif

// -----------------------------------------------------------------------------
// CScenetoneAppView::SizeChanged()
// Called by framework when the view size is changed.
// -----------------------------------------------------------------------------
//
void CScenetoneAppView::SizeChanged()
    {
#if defined(SCENETONE_INCLUDE_VISUALIZER)
    	iScenetone3D->UpdateViewport( Size().iWidth, Size().iHeight );
#else
	DrawNow();   
#endif
 }

TInt CScenetoneAppView::CountComponentControls() const
    {
	return 0;
    }

CCoeControl* CScenetoneAppView::ComponentControl( TInt aIndex ) const
    {
	//if( aIndex == 0)    return iProgress;
	//else                return NULL;
	return NULL;
    }

void CScenetoneAppView::HandleResourceChange(TInt aType)
{
	//if( aType == KEikDynamicLayoutVariantSwitch )
     //  	SetExtentToWholeScreen();
}

void CScenetoneAppView::SetAppUi(CScenetoneAppUi* aUi)
{
  iAppUi = aUi;
}    




#if defined(BUILD_5TH_EDITION_VERSION)
void CScenetoneAppView::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
    // Check if touch is enabled or not
    if( !AknLayoutUtils::PenEnabled() )
    {
      return;
    }

    if(aPointerEvent.iType == TPointerEvent::EDrag)
    {
      if(!iReceivingDrag)
      {
	iReceivingDrag = ETrue;
	
	// this is the first time -> pick up the starting point on the screen
	iDragStart = aPointerEvent.iPosition;

	// if we were doing song drag before AND the timer did NOT fire, we continue previous drag even if the
	// drag was released... otherwise we just check if it is a new drag
	if(isongnamerect.Contains(iDragStart) || iSongDrag)
	{
	  iDragInitialSong = iAppUi->iPlayIndex;
	  iDragLastSong    = iAppUi->iPlayIndex;
	  iSongDrag        = ETrue;
	}
	else if(iswvolrect.Contains(iDragStart))
	{
	  // capture initial volume
	  iDragInitialVolume = iAppUi->iSWVolume;
	  iSWVolDrag = ETrue;
	}
	else if(ihwvolrect.Contains(iDragStart))
	{
	  iDragInitialVolume = iAppUi->iVolume;
	  iHWVolDrag = ETrue;
	}
      }
      else
      {
	// this is the continued drag event

	TPoint delta = aPointerEvent.iPosition - iDragStart;

	if(iSWVolDrag)
	{
	  iAppUi->iSWVolume = iDragInitialVolume + delta.iX;
	  if(iAppUi->iSWVolume < 0)   iAppUi->iSWVolume = 0;
	  if(iAppUi->iSWVolume > 256) iAppUi->iSWVolume = 256;

          iAppUi->iSound->SetSWVolume(iAppUi->iSWVolume);
          iAppUi->UpdateInfo();
	}
	else if(iHWVolDrag)
	{
	  iAppUi->iVolume = iDragInitialVolume + (delta.iX>>1);

	  if(iAppUi->iVolume < 0)   iAppUi->iVolume = 0;
	  if(iAppUi->iVolume > 100) iAppUi->iVolume = 100;

	  iAppUi->iSound->SetVolume(iAppUi->iVolume);
	  iAppUi->UpdateInfo();
	}
	else if(iSongDrag)
	{
	  TInt dsong = iDragInitialSong + (delta.iY >> 4);
	  if(dsong != iDragLastSong)
	  {
	    iAppUi->DirectJumpSong(dsong, ETrue);
	  }

	  iDragLastSong = dsong;
	  
	  // stop previous drag callback (if exists) if we are doing song selection...
	  if(iDragTimer)
	  {
	    if(iDragTimer->IsAdded())  iDragTimer->Deque();
	    delete iDragTimer;
	  }
      
	  // start drag callback to "deactivate" drag, 2 second timeout
	  iDragTimer = CPeriodic::NewL(CActive::EPriorityIdle);
	  iDragTimer->Start(2000000, 2000000, iCallback);
	}
	else return;
      }
    }
    else if(aPointerEvent.iType == TPointerEvent::EButton1Up)
    {
      if(iReceivingDrag)
      {
	iReceivingDrag = EFalse;
	iHWVolDrag     = EFalse;
	iSWVolDrag     = EFalse;
	DrawNow();
      }
    }

    else if(aPointerEvent.iType == TPointerEvent::EButton1Down)
    {
      // check if user pressed the top right corner.. indicates a marker startup
      TRect topright = TRect(180,0,360,180);

      if(topright.Contains(aPointerEvent.iPosition))
      {
	iAppUi->HandleGrabKey();
      }
      else if(ileftselrect.Contains(aPointerEvent.iPosition))
      {
	iAppUi->JumpSong(EKeyLeftArrow);
      }
      else if(irightselrect.Contains(aPointerEvent.iPosition))
      {
	iAppUi->JumpSong(EKeyRightArrow);
      }
      else if(itopselrect.Contains(aPointerEvent.iPosition))
      {
	iAppUi->JumpSong(EKeyUpArrow);
      }
      else if(ibottomselrect.Contains(aPointerEvent.iPosition))
      {
	iAppUi->JumpSong(EKeyDownArrow);
      }
    }
    
    CCoeControl::HandlePointerEventL(aPointerEvent);
}

#endif

