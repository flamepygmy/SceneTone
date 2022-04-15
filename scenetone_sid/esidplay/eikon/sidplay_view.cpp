// sidplay_view.cpp
//
// Copyright (c) 2001-2002 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//

/**
 * @file sidplay_view.cpp
 *
 * Implements concrete control for UI view (CSidPlayerStatusView and CSidPlayAppView)
 */

#include "sidplay_view.h"
#include <sidplay.rsg>

// Eikon
#include <eikclbd.h>

//
// implementation of CSidPlayerStatusView
//


void CSidPlayerStatusView::ConstructL(const TRect& aRect, CCoeControl* aParent)
/**
 * safe constructor
 */
	{
	CTOR(CSidPlayerStatusView);
	iParent = aParent;

	CreateWindowL(aParent);
#if defined(__ER6__)
	SetRect(aRect);
#else
	SetRectL(aRect);
#endif

	// standard font
//	iNormalFont=iEikonEnv->NormalFont();
	}


CSidPlayerStatusView::~CSidPlayerStatusView()
/**
 * D'tor
 */
	{
	DTOR(CSidPlayerStatusView);
	}


void CSidPlayerStatusView::Draw(const TRect& /*aRect*/) const
/**
 * draw the status view area
 */
	{
	ELOG1(_L8("CSidPlayerStatusView::Draw\n"));

	const TInt offset_x = 10;   // X position of e.g. "Name"
	const TInt offset_x2 = 100; // X position of e.g. "Commando"
	const TInt offset_y = 20;
	const TInt distance_y = 20;
	TInt pos_y = offset_y;

	sidTune& tune = ((CSidPlayAppView*)iParent)->SidPlayer()->CurrentSidTune();
	struct sidTuneInfo mySidInfo;
	tune.getInfo(mySidInfo);

	CWindowGc& gc = SystemGc();
	// surrounding rectangle
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.SetBrushColor(KRgbWhite);
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetPenColor(KRgbBlack);
	gc.SetBrushStyle(CGraphicsContext::ENullBrush);

	// some handy variables
	TBuf<128> buf;
	TBuf<128> rs; // for reading resources

	// Area in which we shall draw
	TRect drawRect = Rect();

	// Font used for drawing text
	const CFont* fontUsed;

	// UI environment
	CEikonEnv* eikonEnv = CEikonEnv::Static();

	// Start with a clear screen
	gc.Clear();

	// Draw an outline rectangle (the default pen
	// and brush styles ensure this) slightly
	// smaller than the drawing area.
	drawRect.Shrink(5, 5);
	gc.DrawRect(drawRect);

	// Use the title font supplied by the UI
	fontUsed = eikonEnv->TitleFont();
	gc.UseFont(fontUsed);

	if(mySidInfo.nameString)
		{
		gc.DrawText(_L("Name"), TPoint(offset_x, pos_y));
		buf.Copy(TPtrC8((TUint8*)mySidInfo.nameString));
		gc.DrawText(buf, TPoint(offset_x2, pos_y));
		pos_y += distance_y;
		}

	if(mySidInfo.authorString)
		{
		gc.DrawText(_L("Author"), TPoint(offset_x, pos_y));
		buf.Copy(TPtrC8((TUint8*)mySidInfo.authorString));
		gc.DrawText(buf, TPoint(offset_x2, pos_y));
		pos_y += distance_y;
		}

	if(mySidInfo.copyrightString)
		{
		gc.DrawText(_L("Copyright"), TPoint(offset_x, pos_y));
		buf.Copy(TPtrC8((TUint8*)mySidInfo.copyrightString));
		gc.DrawText(buf, TPoint(offset_x2, pos_y));
		pos_y += distance_y;
		}

	gc.DrawText(_L("Subtune"), TPoint(offset_x, pos_y));
	rs.Format(_L("%d/%d"), mySidInfo.currentSong, mySidInfo.songs);
	gc.DrawText(rs, TPoint(offset_x2, pos_y));
	pos_y += distance_y;

	// clear GC's font
	gc.DiscardFont();
	}


//
// implementation of CSidPlayerTimeView
//


TInt SidPlayerTimeViewPeriodicUpdate(TAny* aPtr)
/**
 * Called every second by periodic timer
 */
	{
	CSidPlayerTimeView* view = (CSidPlayerTimeView*)aPtr;
	if(((CSidPlayAppView*)(view->iParent))->SidPlayer()->iIdlePlay)
		view->DrawNow();
	return ETrue;
	}


void CSidPlayerTimeView::ConstructL(const TRect& aRect, CCoeControl* aParent)
/**
 * safe constructor
 */
	{
	iParent = aParent;

	CreateWindowL(aParent);
#if defined(__ER6__)
	SetRect(aRect);
#else
	SetRectL(aRect);
#endif

	iPeriodic = CPeriodic::NewL(CActive::EPriorityLow);
	TCallBack cb(SidPlayerTimeViewPeriodicUpdate, this);
	iPeriodic->Start(TTimeIntervalMicroSeconds32(0),
			TTimeIntervalMicroSeconds32(500000),
				cb);
	}


CSidPlayerTimeView::~CSidPlayerTimeView()
/**
 * D'tor
 */
	{
	if(iPeriodic)
		{
		delete iPeriodic;
		iPeriodic = NULL;
		}
	}


void CSidPlayerTimeView::Draw(const TRect& /*aRect*/) const
/**
 * draw the status view area
 */
	{
	const TInt offset_x = 10;   // X position of e.g. "Name"
	const TInt offset_x2 = 60; // X position of e.g. "Commando"
	const TInt offset_y = 20;
	const TInt distance_y = 20;
	TInt pos_y = offset_y;

	emuEngine* ee = ((CSidPlayAppView*)iParent)->SidPlayer()->iEmuEngine;

	CWindowGc& gc = SystemGc();
	// surrounding rectangle
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.SetBrushColor(KRgbWhite);
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetPenColor(KRgbRed);
	gc.SetBrushStyle(CGraphicsContext::ENullBrush);

	// some handy variables
	TBuf<128> buf;
	TBuf<128> rs; // for reading resources

	// Area in which we shall draw
	TRect drawRect = Rect();

	// Font used for drawing text
	const CFont* fontUsed;

	// UI environment
	CEikonEnv* eikonEnv = CEikonEnv::Static();

	// Start with a clear screen
	gc.Clear();

	// Draw an outline rectangle (the default pen
	// and brush styles ensure this) slightly
	// smaller than the drawing area.
	drawRect.Shrink(5, 5);
	gc.DrawRect(drawRect);

	// Use the title font supplied by the UI
	fontUsed = eikonEnv->TitleFont();
	gc.UseFont(fontUsed);

	gc.DrawText(_L("Time"), TPoint(offset_x, pos_y));
	rs.Format(_L("%d:%02d"), ee->getSecondsThisSong() / 60, ee->getSecondsThisSong() % 60);
	gc.DrawText(rs, TPoint(offset_x2, pos_y));
	pos_y += distance_y;

	gc.DrawText(_L("Total"), TPoint(offset_x, pos_y));
	rs.Format(_L("%d:%02d"), ee->getSecondsTotal() / 60, ee->getSecondsTotal() % 60);
	gc.DrawText(rs, TPoint(offset_x2, pos_y));
	pos_y += distance_y;

	// clear GC's font
	gc.DiscardFont();
	}


//
// implementation of CSidPlayAppView
//


void CSidPlayAppView::ConstructL(const TRect& aRect, CSidPlayer* aThePlayer)
/**
 * safe constructor
 */
	{
	CTOR(CSidPlayAppView);

	iThePlayer = aThePlayer;
	CreateWindowL();

#if defined(__CRYSTAL__)
	HBufC* appName = CEikonEnv::Static()->AllocReadResourceLC(R_SIDPLAY_APPNAME);

	// Application title
	iAppTitle = CCknAppTitle::NewL();
	iAppTitle->SetTextL(*appName,CCknAppTitle::EMainTitle);
	CleanupStack::PopAndDestroy();//appName;
	TSize titleSize(aRect.Width(),iAppTitle->MinimumSize().iHeight);
	TRect titleRect(TPoint(0,0),titleSize);

	iAppTitle->SetRect(titleRect);
	iAppTitle->SetFocus(ETrue);
#else
	TRect titleRect(TPoint(0,0), TPoint(0,0));
#endif // __CRYSTAL__

	/*
	 * UI layout
	 */
#if defined(__SERIES60__)
	const TRect status_view = TRect(1, 1, aRect.Width(), aRect.Height()-60);
	const TRect time_view = TRect(1, aRect.Height()-60+1, aRect.Width(), aRect.Height()-20);
#else // CRYSTAL
	const TRect status_view = TRect(1, titleRect.Height(), 360, aRect.Height());
	const TRect time_view = TRect(360, titleRect.Height(), aRect.Width(), titleRect.Height()+60);
	const TRect volumeRect = TRect(400, titleRect.Height()+60 , aRect.Width()-1, aRect.Height()-1-20);
#endif


	//
	// set up status view
	//
	iStatusView = new (ELeave) CSidPlayerStatusView;
	iStatusView->ConstructL(status_view, this);

	//
	// set up time view
	//
	iTimeView = new (ELeave) CSidPlayerTimeView;
	iTimeView->ConstructL(time_view, this);


	//
	// set up volume view
	//
#if defined(__CRYSTAL__)
	CCknControlBar::SControlBarData v;
	v.iHeight = aRect.Width() - 1 - 441; // x
	v.iWidth = aRect.Height() - 1 - 81 - 20;  // y
	v.iFinalValue = KVolumeSteps - 1;

	iVolumeView = new (ELeave) CCknControlBar(v, EFalse);

	CleanupStack::PushL(iVolumeView);
	iVolumeView->SetContainerWindowL(*this);
	CleanupStack::Pop(iVolumeView);

	iVolumeView->ActivateL();
	iVolumeView->SetRect(volumeRect);

	_LIT(KVolumeIcon, "\\system\\apps\\sidplay\\e32frodo.mbm");
	CGulIcon* icon = iEikonEnv->CreateIconL(KVolumeIcon, 0 /* bitmap ID */);
	iVolumeView->SetIcon(icon,ETrue); // let the control bar own the icon

	const TInt vol = iThePlayer->VolumeDelta(0);
	iVolumeView->SetAndDraw(vol);
#endif // __CRYSTAL__

#if defined(__ER6__)
	SetRect(aRect);
#else
	SetRectL(aRect);
#endif

	// activate
	ActivateL(); // ready for drawing etc
	}


CSidPlayAppView::~CSidPlayAppView()
/**
 * D'tor
 */
	{
	DTOR(CSidPlayAppView);
	delete iTimeView;
	delete iStatusView;
#if defined(__CRYSTAL__)
	delete iAppTitle;
	delete iVolumeView;
#endif
	}


void CSidPlayAppView::Draw(const TRect& /*aRect*/) const
/**
 * draw the client rectangular area
 */
	{
	}


TKeyResponse CSidPlayAppView::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
	{
	// don't handle keys when this view is hidden
//	if (!IsVisible())
		return EKeyWasNotConsumed;
	}


TInt CSidPlayAppView::CountComponentControls() const
/**
 * return number of component controls in our control
 */
	{
#if defined(__CRYSTAL__)
	return 4;
#else
	return 2;
#endif
	}


CCoeControl* CSidPlayAppView::ComponentControl(TInt aIndex) const
/**
 * return the pointer to the control given in the index
 */
	{
	switch (aIndex)
		{
	case 0: return iStatusView;
	case 1: return iTimeView;
#if defined(__CRYSTAL__)
	case 2: return iAppTitle;
	case 3: return iVolumeView;
#endif
		}
	return 0;
	}
