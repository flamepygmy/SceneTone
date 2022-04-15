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

#ifndef __SCENETONEAPPUI_H__
#define __SCENETONEAPPUI_H__

// INCLUDES
#include <eikon.hrh>
#include <eikprogi.h>
#include <aknappui.h>
#include "scenetonesound.h"
#include "settingsutil.h"

// FORWARD DECLARATIONS
class CScenetoneAppView;


// CLASS DECLARATION
/**
* CScenetoneAppUi application UI class.
* Interacts with the user through the UI and request message processing
* from the handler class
*/
class CScenetoneAppUi : public CAknAppUi
    {
    public: // Constructors and destructor

        /**
        * ConstructL.
        * 2nd phase constructor.
        */
        void ConstructL();

        /**
        * CScenetoneAppUi.
        * C++ default constructor. This needs to be public due to
        * the way the framework constructs the AppUi
        */
        CScenetoneAppUi();

        /**
        * ~CScenetoneAppUi.
        * Virtual Destructor.
        */
        virtual ~CScenetoneAppUi();

	

    private:  // Functions from base classes

	TKeyResponse HandleKeyEventL(    const TKeyEvent& aKeyEvent, TEventCode aType );


        /**
        * From CEikAppUi, HandleCommandL.
        * Takes care of command handling.
        * @param aCommand Command to be handled.
        */
        void HandleCommandL( TInt aCommand );

        /**
        *  HandleStatusPaneSizeChange.
        *  Called by the framework when the application status pane
 		*  size is changed.
        */

		void HandleStatusPaneSizeChange();
        
    private: // Data

        /**
        * The application view
        * Owned by CScenetoneAppUi
        */
	
	
    public:

	TInt            iVolume;
	TInt            iSWVolume;

	TBool           iConverting;
        CScenetoneAppView* iAppView;
	void GetFileAndPlay(TInt aDefault);
	void AskTimeAndGenerate();
	void UpdateInfo();
	void Callback();
	void JumpSong(TUint code);
	void DirectJumpSong(TInt aIndex, TBool aJustInfo);
	void HandleGrabKey();

	CScenetoneSound	*iSound;
	CIdle           *iIdle;
	TBool			 iFirstIdle;
	CEikProgressInfo *iProgress;

	TFileName       iDefaultPath;
	TFileName	iCurrentlyPlaying;	
	CSettingsUtil   *iSettingsUtil;
	CDir		*iCurDir;
	TInt             iPlayIndex;

	CPeriodic        *iDelayedStart;
	TCallBack        iDelayedCallback;

	TReal            iPlayStartRecording;
	TReal            iPlayStopRecording;
    };

#endif // __SCENETONEAPPUI_H__

// End of File
