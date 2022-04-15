// sidplay_view.h
//
// Copyright (c) 2001-2002 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//

/**
 * @file sidplay_view.h
 *
 * Declares concrete control for UI view (CSidPlayerStatusView and CSidPlayAppView)
 */
#if !defined(SIDPLAY_VIEW_H__)
#define SIDPLAY_VIEW_H__


// Eikon
#include <eikenv.h>
#include <eikclb.h>

// Cone
#include <coecntrl.h>
#include <coeutils.h>
#if !defined(__ER6__)
#include <coealign.h>
#endif

// Ckon
#if defined(__CRYSTAL__)
#include <ckntitle.h>
#include <cknctbar.h>
#endif

// Avkon
#if defined(__SERIES60__)
#include <aknview.h>
#endif

#include "sidplayer.h"

/**
 * view the status
 */
class CSidPlayerStatusView : public CCoeControl
	{
public:
	~CSidPlayerStatusView();
	void ConstructL(const TRect& aRect, CCoeControl* aParent);
private: // from CCoeControl
	void Draw(const TRect&) const;
private:
//	const CFont* iNormalFont;
	CCoeControl* iParent;   ///< the guy who owns us
	};


extern TInt SidPlayerTimeViewPeriodicUpdate(TAny* aPtr);

/**
 * view the time
 */
class CSidPlayerTimeView : public CCoeControl
	{
	friend TInt SidPlayerTimeViewPeriodicUpdate(TAny* aPtr);
public:
	~CSidPlayerTimeView();
	void ConstructL(const TRect& aRect, CCoeControl* aParent);
private: // from CCoeControl
	void Draw(const TRect&) const;
private:
	CCoeControl* iParent;   ///< the guy who owns us
	CPeriodic* iPeriodic;   ///< periodic timer to update view
	};


/**
 * CSidPlayAppView - how you see it
 */
//#if defined(__SERIES60__)
//class CSidPlayAppView : public CAknView
//#else
class CSidPlayAppView : public CCoeControl
//#endif
	{
	friend class CSidPlayAppUi;
public:
	~CSidPlayAppView();
	void ConstructL(const TRect& aRect, CSidPlayer* aThePlayer);
	CSidPlayer* SidPlayer() const{return iThePlayer;}
private:
	// from CCoeControl
	void Draw(const TRect&) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
private:
	// copy of stuff elsewhere
	CSidPlayer* iThePlayer;
//	CEikColumnListBox* iPlayList;
	CSidPlayerTimeView* iTimeView;
	CSidPlayerStatusView* iStatusView;
#if defined(__CRYSTAL__)
	CCknAppTitle* iAppTitle;
	CCknControlBar* iVolumeView;
#endif
	};


#endif // SIDPLAY_VIEW_H__
