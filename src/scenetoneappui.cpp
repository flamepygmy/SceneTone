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

/***************************************************************************************************************************
 * Version history *
 *******************

    v0.1    few weeks before christmas 2005    

    v0.2    5.1.2006          Multithreaded, multibuffered version. No sound skipping etc.

    v0.3    4.3.2006          MP3 generation through temp .WAV file

    v0.4    4.4.2006	      Plugin based architecture, but linked into one application (carbide issue with
			      linking ESTLIB...)
							  
    v0.5    13.4.2006	      Subsong selection, new UI for wav generation: play tune, change subsong etc.., then
			      "generate wav from current song". Ask for wav generation length in seconds.

  --- Version history under GPL


    v0.6    5.8.2006	      Keep playing song while selecting next song (multitask) + display currently playing song +
			      first try at adding SVG based icon (gradients not working). Also, added small OpenGL ES
			      visualizer (visualizer is not ready yet, missing stuff, but it displays something ->
			      framework is there). Removed MP3 due to licensing issues and moved back to .WAV generation
			      instead. Added GPL info everywhere, this SW is now under GPL. This version is the first
			      "snapshot" version to go to public distribution.

    v0.7   7.8.2006           Allocated an UID from Symbian for this project from the unprotected range and changed the SW to use
                              it. Also, OpenGL ES visualizer is disabled by default (for now, until it is ready and can be
                              either scaled to device 3D performance and/or turned on/off from the UI).

    v0.8   29.8.2006          Implemented settings file, where one can store the default folder to open when "open module"
     			      is selected. Changed file open code to keep the folder from "open module" to another (this
     			      is "per-session" last used directory, which is initialized from the default path stored in
     			      settings file). Also implemented "start prev/next module" functionality. Pressing "4" starts
     			      previous module and "6" the next module in the file list of the last visited directory. Wrapping
     			      is implemented also. Also added more error checking (should do it properly all around in the
     			      SW) to not crash for SID songs that just dont work. Better way would be to fix the issues
     			      with those songs OR to show that initialization failed instead just being silent.

	v0.9	20.12.2006	Better cleanup from sid and mikmod modules -> should not leak memory (as much
				at least anymore). Updated 
						

    v0.10   29.04.2008     Key changes: '4'/'6' keys control hard volume now, '2'/'8' SW volume up/down,
                                            left/right => next/prev subsong, left/right => prev/next subsong.
                                            open from default folder vs. open (generic). Record time capture by
                                            pressing selection key (center of joystick). See full list of changes
                                            from Changelog.

    v0.11    xx.yy.2009    Touch / S60 5.0 support.    


For next versions:

    - split the S60 5Th -> 5th touch vs. non-touch?

    - support for different resolutions / layouts

    - check libsidplayv2, if that would port nicely (with decent performance)

    - handle following case: press "next song" after WAV generation (crashes for some reason)
	
    - if mikmod supports "end of song" somehow, implement "move to next song" if previous ends (default behaviour? configurable?)
         * for looping mods: you are SOL
         * for SIDS: configurable timeout

    - add support for changing the default .WAV generation path (store in settings file)

    - randomized play

    - recursive dir scan

    - support for 2nd edition phones (UI vs. player thread/server cleanup, 2nd ed: server process and ui
      process instead of server thread and ui thread in 3rd edition... reason: globals...)
		
*/

// TODO: error checking + cleanup + "if not initialized" guards everywhere where necessary
// TODO: cleaner progress bar implementation (global one is not OK)
// TODO: display more info on song (subsong number, seconds played/max time, also display in "idle screen" if possible, like
//       the standard audio player does...)
// TODO: 2-pass AAC/AAC+ encoding support (MMF API allows to compress from file -> file, but not on-the-fly)
// TODO: because of the OpenGL ES visualizer now the "currently playing" is not actually displayed anymore ...
//       -> should do that in ES side as well
// TODO: add ogg/vorbis support
// TODO: S60 2nd version (global stuff is anyway behind client-server comms at the moment.. however, some data is
//       exchanged between threads in incorrect way -> clean/abstract that out first)
// TODO: täytyy requestin uusiminen odottaakkin ("ok") ettei vahingossa issueta useampia komentoja peräkkäin -> stray

// INCLUDE FILES
#include <avkon.hrh>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <scenetone.rsg>
#include <f32file.h>
#include <s32file.h>
#include <utf.h>
#include <e32base.h>

#include <eikon.hrh>
#include <eikprogi.h>
#include <eikinfo.h> 

#include <akncommondialogs.h> 
#include <caknmemoryselectiondialog.h> 
#include <caknfileselectiondialog.h> 
#include <pathinfo.h>

#include "scenetonesound.h"

#include "scenetone.pan"
#include "scenetoneappui.h"
#include "scenetoneappview.h"
#include "scenetone.hrh"

#include "settingsutil.h"

_LIT(KProsenttiAs, "%S");
_LIT(KSpacedOut, " ");
_LIT(KScenetoneVersion, 			"Scenetone v0.11");
_LIT(KAskFile,                                  "Select file");
_LIT(KAskMemory,                                "Select memory");
_LIT(KScenetoneSettingsFile, 			"\\private\\A0001186\\settings.bin");
_LIT(KScenetoneSettingKeyDefaultPath, 	        "MODPATH");
_LIT(KScenetoneDefaultPath, 			"e:\\myown\\mods");

// Note: actual writer will append the tail ".wav" or ".mp3" etc..
_LIT(KWavDestination, 				"c:\\data\\sounds\\digital\\scenetone");
_LIT(KNowPlayingFormatString,  			"%S");

static TInt MyCallback(TAny *ptr)
{
  CScenetoneAppUi *t = (CScenetoneAppUi *)ptr;

  TBuf<512> fname;
  fname = t->iDefaultPath;
  fname.Append(t->iCurrentlyPlaying);

  t->iAppView->iScenetone3D->Stop();
  t->iSound->StopL();

  t->iSound->SetFileName(fname);
  t->iSound->StartL(); 

  t->iPlayStartRecording = 0.f;
  t->iPlayStopRecording  = 0.f;
  t->iAppView->iScenetone3D->Start();

  t->iDelayedStart->Deque();

  return 0;
}



// ============================ MEMBER FUNCTIONS ===============================


// -----------------------------------------------------------------------------
// CScenetoneAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//

void CScenetoneAppUi::ConstructL()
    {
    // Initialise app UI with standard value.
    BaseConstructL();

    iAppView = CScenetoneAppView::NewL( ClientRect() );
    iAppView->SetMopParent(this);
    AddToStackL(iAppView);

    iSound = CScenetoneSound::NewL();
    iSound->SetVisualizer(iAppView->iScenetone3D);
    iVolume = 50;
    iSWVolume = 256;
    iConverting = EFalse;
    //iSound->SetVolume(iVolume);

    // Load and parse settings file (default file part of SIS file)
    iSettingsUtil = CSettingsUtil::NewL();
    TInt err = iSettingsUtil->ReadSettings(KScenetoneSettingsFile);
    if(!err)
    {
	iSettingsUtil->GetSetting( KScenetoneSettingKeyDefaultPath, iDefaultPath );
    }
    else
    {
	iDefaultPath = 	KScenetoneDefaultPath;
    }

    iDelayedCallback = TCallBack(MyCallback, (TAny*)this);

    iAppView->SetAppUi(this);

    SetOrientationL(EAppUiOrientationPortrait);

}


// -----------------------------------------------------------------------------
// CScenetoneAppUi::CScenetoneAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CScenetoneAppUi::CScenetoneAppUi()
    {
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CScenetoneAppUi::~CScenetoneAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CScenetoneAppUi::~CScenetoneAppUi()
    {
    if ( iAppView )
        {
        delete iAppView;
        iAppView = NULL;
        }

	if(iCurDir) delete iCurDir;

    }

_LIT(KInfoFormatString, "HW VOL: %d/100, SW VOL: %d/256");

void CScenetoneAppUi::UpdateInfo()
{
  iCurrentlyPlaying = (*iCurDir)[iPlayIndex].iName;
  
  iAppView->iPlayInfo.Format(KInfoFormatString, iVolume, iSWVolume);

#if defined(BUILD_5TH_EDITION_VERSION)

    int i;
    for(i=0;i<iAppView->iBrowserSongCount;i++) iAppView->iBrowserSongs[i].Format(KSpacedOut);

    int cnt  = iAppView->iBrowserSongCount;
    int off  = iPlayIndex - iAppView->iBrowserSongCurrentIndex;

    for(i=0;i<cnt;i++)
    {
      if(((off+i) >= 0) && ((off+i) < iCurDir->Count()))
	iAppView->iBrowserSongs[i].Format(KNowPlayingFormatString, &((*iCurDir)[off+i].iName));
    }

    iAppView->iPlayText.Format (KNowPlayingFormatString,  &iCurrentlyPlaying);

#else
    iAppView->iPlayText.Format (KNowPlayingFormatString,  &iCurrentlyPlaying);
    iAppView->iPlayMinusTwo.Format(KSpacedOut);
    iAppView->iPlayMinusOne.Format(KSpacedOut);
    iAppView->iPlayPlusOne.Format(KSpacedOut);
    iAppView->iPlayPlusTwo.Format(KSpacedOut);

    if(iPlayIndex > 1)
    {
      iAppView->iPlayMinusTwo.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex-2].iName));
    }
    if(iPlayIndex > 0)
    {
      iAppView->iPlayMinusOne.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex-1].iName));
    }
    if(iPlayIndex < (iCurDir->Count()-1))
    {
      iAppView->iPlayPlusOne.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex+1].iName));
    }
    if(iPlayIndex < (iCurDir->Count()-2))
    {
      iAppView->iPlayPlusTwo.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex+2].iName));
    }
#endif

#if !defined(SCENETONE_SUPPORT_VISUALIZER)
    iAppView->DrawNow();
#endif
}

void CScenetoneAppUi::DirectJumpSong(TInt aIndex, TBool aJustInfo)
{
  iPlayIndex = aIndex;
  if(iPlayIndex < 0) 			      iPlayIndex = iCurDir->Count()-1;
  if(iPlayIndex > (iCurDir->Count()-1))       iPlayIndex = 0;

  UpdateInfo();
  if(aJustInfo) return;

  if(iDelayedStart)
  {
    if(iDelayedStart->IsAdded())  iDelayedStart->Deque();
    delete iDelayedStart;
  }
  iDelayedStart = CPeriodic::NewL(CActive::EPriorityIdle);
  iDelayedStart->Start(1, 1, iDelayedCallback);
}

void CScenetoneAppUi::JumpSong(TUint aKeyCode)
{
  if( aKeyCode == EKeyRightArrow )
  {
    iAppView->iScenetone3D->Stop();
    iSound->NextSubsong();
    iAppView->iScenetone3D->Start();
    return;
  }
  else if( aKeyCode == EKeyLeftArrow )
  {
    iAppView->iScenetone3D->Stop();
    iSound->PrevSubsong();
    iAppView->iScenetone3D->Start();
    return;
  }

  if((iCurrentlyPlaying.Length() > 0) && iCurDir)
  {
    // prev/next song in directory
    TInt adder = -1;
    if(aKeyCode == EKeyDownArrow)		adder = 1;

    iPlayIndex = iPlayIndex + adder;

    if(iPlayIndex < 0) 				iPlayIndex = iCurDir->Count()-1;
    if(iPlayIndex > (iCurDir->Count()-1))       iPlayIndex = 0;

    // new version: use deferred callback to trigger the play (makes scrolling faster)
    if(iDelayedStart)
    {
      if(iDelayedStart->IsAdded())  iDelayedStart->Deque();
      delete iDelayedStart;
    }
	  
    iDelayedStart = CPeriodic::NewL(CActive::EPriorityIdle);
#if defined(BUILD_5TH_EDITION_VERSION)
    iDelayedStart->Start(1000000, 1000000, iDelayedCallback);
#else
    iDelayedStart->Start(500000, 500000, iDelayedCallback);
#endif

    UpdateInfo();

#if !defined(SCENETONE_SUPPORT_VISUALIZER)
    iAppView->DrawNow();
#endif
  }
}

void CScenetoneAppUi::HandleGrabKey()
{
  if(iAppView->iTakingRecordTimes == 0)
  {
    // grab start time
    iPlayStartRecording = iSound->GetOffset();
    iAppView->iTakingRecordTimes = 1;
#if !defined(SCENETONE_SUPPORT_VISUALIZER)
    iAppView->DrawNow();
#endif
  }
  else
  {
    // grab end time
    iPlayStopRecording = iSound->GetOffset();
    iAppView->iTakingRecordTimes = 0;

#if !defined(SCENETONE_SUPPORT_VISUALIZER)
    iAppView->DrawNow();
#endif
  }
}


TKeyResponse CScenetoneAppUi::HandleKeyEventL(
    const TKeyEvent& aKeyEvent, TEventCode aType )
{
	if(iConverting) return EKeyWasNotConsumed;

    if( aType == EEventKey)
	{
	  if( ( aKeyEvent.iCode == '4' ))
	  {
	    if(iVolume <= 10)   iVolume--;
	    else                iVolume -= 10;
	    if(iVolume < 0) iVolume = 0;
	    iSound->SetVolume(iVolume);

        UpdateInfo();
	    return EKeyWasConsumed;
	  }
	  else if( ( aKeyEvent.iCode == '6' ))
	  {
        if(iVolume < 10)    iVolume++;
        else                iVolume += 10;
        if(iVolume > 100) iVolume = 100;
        iSound->SetVolume(iVolume);

        UpdateInfo();
        return EKeyWasConsumed;
	  }
	  else if( ( aKeyEvent.iCode == '2' ))
	  {
          iSWVolume += 16;
          if(iSWVolume > 256) iSWVolume = 256;
          iSound->SetSWVolume(iSWVolume);
 
          UpdateInfo();
          return EKeyWasConsumed;
	  }
      else if( ( aKeyEvent.iCode == '8' ))
      {
          iSWVolume -= 16;
          if(iSWVolume < 0) iSWVolume = 0;
          iSound->SetSWVolume(iSWVolume);
 
          UpdateInfo();
          return EKeyWasConsumed;
      }
	        else if( aKeyEvent.iCode == EKeyDevice3 )
		{
		  HandleGrabKey();
		  return EKeyWasConsumed;
		}
		else if( aKeyEvent.iCode == EKeyUpArrow   || aKeyEvent.iCode == EKeyDownArrow ||
			 aKeyEvent.iCode == EKeyLeftArrow || aKeyEvent.iCode == EKeyRightArrow)
		{
		  JumpSong(aKeyEvent.iCode);
		  return EKeyWasConsumed;
		}
	}
    return EKeyWasNotConsumed;
}

// open_bit_stream_w -> remove bitstream.c, functions defined here
// close_bit_stream
// putbits

// need wave_get     -> remove wave.c


void CScenetoneAppUi::AskTimeAndGenerate()
{
  /* The old way... */
  if((iPlayStartRecording == 0.f) && (iPlayStopRecording == 0.f))
  {
	TInt itime = 35;
        CAknNumberQueryDialog *dlg = new (ELeave) CAknNumberQueryDialog( itime );
	dlg->PrepareLC( R_SCENETONE_GENERATE_TIME_QUERY );

	if(dlg->RunLD())
	{
		iAppView->iScenetone3D->Stop();
		iSound->GenerateWav(KWavDestination, 0, itime);
	}
  }
  else
  {
    /* the new way */
    iAppView->iScenetone3D->Stop();

    iSound->GenerateWav(KWavDestination, iPlayStartRecording, iPlayStopRecording);
  }
}

void CScenetoneAppUi::GetFileAndPlay(TInt aDefault)
{
    TBuf8<KMaxFileName+1> fname8;
    TFileName fname;

    if(!aDefault)
    {
        iDefaultPath.Format(KSpacedOut);
    }
	iAppView->iScenetone3D->Stop();

	// first check if the default folder path exists..
	RFs session;
	session.Connect();

	RDir dir;
	if((dir.Open(session, iDefaultPath, KNullUid) != KErrNone)||(iDefaultPath.Length() == 0))
	{
	  // directory does not exist => ask for the full thing

	  CAknMemorySelectionDialog::TMemory mem = CAknMemorySelectionDialog::EMemoryCard;
	  CAknMemorySelectionDialog *mdlg = CAknMemorySelectionDialog::NewL(ECFDDialogTypeNormal, EFalse);
	  mdlg->SetTitleL(KAskMemory);
	  mdlg->RunDlgLD(mem, NULL);

	  if(mem == CAknMemorySelectionDialog::EMemoryCard)
	  {
	    iDefaultPath.Format(KProsenttiAs, &PathInfo::MemoryCardRootPath());
	  }
	  else
	  {
	    iDefaultPath.Format(KProsenttiAs, &PathInfo::PhoneMemoryRootPath());
	  }
	}
	dir.Close();

	fname.Format(KProsenttiAs, &iDefaultPath.Left(3));
	CAknFileSelectionDialog *fdlg = CAknFileSelectionDialog::NewL(ECFDDialogTypeNormal);

	if(iDefaultPath.Length() < 4)
	{
	  fdlg->RunDlgLD(fname, KSpacedOut, KAskFile, NULL);
	}
	else
	{
	  fdlg->RunDlgLD(fname, iDefaultPath.Right(iDefaultPath.Length()-3), KAskFile, NULL);
	}
	
	CnvUtfConverter::ConvertFromUnicodeToUtf8(fname8, fname);
	fname8.Append(TChar('\0'));

	iSound->StopL();
		
    	iSound->SetFileName(fname);
    	iSound->StartL(); 

	iPlayStartRecording = 0.f;
	iPlayStopRecording  = 0.f;

	TParse parse;
	parse.Set(fname, NULL, NULL);

	if((iDefaultPath != parse.DriveAndPath()) || !iCurDir)
	{
	  // folder was changed, store new folder and process the directory tree
	  iDefaultPath = parse.DriveAndPath();		

	  // delete old one
	  if(iCurDir) delete iCurDir;
	  iCurDir = NULL;
	  session.GetDir(iDefaultPath, KEntryAttNormal, ESortByName, iCurDir);
	  
	}
	iCurrentlyPlaying = parse.NameAndExt();
	
	// find the index of the file in the directory

	iPlayIndex = 0;
	while(1)
	{
	  if((*iCurDir)[iPlayIndex].iName == iCurrentlyPlaying)
	  {
	    break;
	  }
	  iPlayIndex++;	
	}

	UpdateInfo();

#if 0
	iAppView->iPlayText.Format (KNowPlayingFormatString,  &parse.Name());
	iAppView->iPlayMinusTwo.Format(KSpacedOut);
	iAppView->iPlayMinusOne.Format(KSpacedOut);
	iAppView->iPlayPlusOne.Format(KSpacedOut);
	iAppView->iPlayPlusTwo.Format(KSpacedOut);
	
	if(iPlayIndex > 1)
	{
	  iAppView->iPlayMinusTwo.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex-2].iName));
	}
	if(iPlayIndex > 0)
	{
	  iAppView->iPlayMinusOne.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex-1].iName));
	}
	if(iPlayIndex < (iCurDir->Count()-1))
	{
	  iAppView->iPlayPlusOne.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex+1].iName));
	}
	if(iPlayIndex < (iCurDir->Count()-2))
	{
	  iAppView->iPlayPlusTwo.Format(KNowPlayingFormatString, &((*iCurDir)[iPlayIndex+2].iName));
	}
#endif

       iAppView->iScenetone3D->Start();

#if !defined(SCENETONE_SUPPORT_VISUALIZER)
		iAppView->DrawNow();
#endif

		session.Close();
}


_LIT(KAboutText, "(c)2006-2009 Jani Vaarala\nLicensed under GPL\npygmyprojects.com");
_LIT(KDefaultFolderStored, "Last folder will be stored as default folder at application exit.");

// -----------------------------------------------------------------------------
// CScenetoneAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CScenetoneAppUi::HandleCommandL( TInt aCommand )
{
	if(iConverting) return;

    switch( aCommand )
    {
        case EEikCmdExit:
        case EAknSoftkeyExit:
	    	iSound->Exit();
	    	delete iSound;

			iSettingsUtil->StoreSettings( KScenetoneSettingsFile );
			
            Exit();
            break;

        case EScenetoneCommand1:
        {
	  //TInt playstate = iSound->State();
            GetFileAndPlay(1);
            break;
		}

        case EScenetoneCommand2:
        {
	  //TInt playstate = iSound->State();
            GetFileAndPlay(0);
            break;
        }

		/* Generate MP3 of the current playing song + subsong */
        case EScenetoneCommand3:
        {
            TInt playstate = iSound->State();
            if(playstate == CScenetoneSound::EPlaying)
            {
                AskTimeAndGenerate();
            }
        }
        break;
        
        /* Set current path as the default path in the future */
        case EScenetoneTakeCurrentFolderAsDefaultPath:
		{
			iSettingsUtil->SetSetting( KScenetoneSettingKeyDefaultPath, iDefaultPath );
			CEikInfoDialog::RunDlgLD( KScenetoneVersion, KDefaultFolderStored );
			break;
		}        	
    
		/* Display ABOUT box */
		case EScenetoneAbout:
		{
			CEikInfoDialog::RunDlgLD( KScenetoneVersion, KAboutText );
		}
		break;
		
        default:
            Panic( EScenetoneUi );
            break;
        }
    }
    
// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CScenetoneAppUi::HandleStatusPaneSizeChange()
{
	iAppView->SetRect( ClientRect() );
	
} 

// End of File

