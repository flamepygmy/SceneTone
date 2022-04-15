// sidplayeik.cpp
//
// Copyright (c) 2000-2002 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//

/**
 * @file sidplayeik.cpp
 *
 * GUI part of SidPlay using EIKON
 */


// Eikon
#include <eikapp.h>
#include <eikappui.h>

#if defined(__CRYSTAL__) // Crystal / CKON
#include <ckndgopn.h> // for CCknOpenFileDialog
#include <ckninfo.h>
#elif defined(__SERIES60__)
#include <aknapp.h>
#include <aknappui.h>
#include <akndoc.h>
#include <aknnotedialog.h>
#include <eikinfo.h>
//#include <filelistdialog.h>
#elif defined(__QUARTZ__)
/// @todo placeholder
#else // ER5
#include <eikbordr.h>
#include <eikcfdlg.hrh>
#include <eikcmds.hrh>
#include <eikdutil.h>
#include <eikfbrow.h>
#endif

#include <eikdef.h>
#include <eikdialg.h>
#include <eikdoc.h>
#include <eikenv.h>

#if defined(__CRYSTAL__) && !defined(__SERIES60__)
#include <eikfsel.h>
#include <eikform.h>
#endif

#include <eiklabel.h>
#include <eikmsg.h>
#include <eikon.rsg>
#include <eiktbar.h>
#include <eikcmbut.h>
#include <eikmenub.h>

// Cone
#include <coecntrl.h>
#include <coeutils.h>
#if !defined(__ER6__)
#include <coealign.h>
#endif


// E32
#include <e32hal.h>
#include <e32keys.h>
#include <basched.h>
#include <apaflrec.h>

// sidplay files
#include <sidplay.rsg>
#include "sidplay.hrh"
#include "elog.h"

#include "sidplay_view.h"
#include "sidplayer.h"

#include "6510_.h"


/*
 * Some local settings
 */
#ifdef __ER6__
#define KMyUid TUid::Uid(0x10009a97);
#else
#define KMyUid TUid::Uid(0x10009a96);
#endif


/**
 * CSidPlayAppUi - Application UI
 */
#if defined(__SERIES60__)
class CSidPlayAppUi : public CAknAppUi
#else
class CSidPlayAppUi : public CEikAppUi
#endif
	{
public:
	void ConstructL();
	~CSidPlayAppUi();
private: // framework
	void DoBrowseFileL();
	void DoScanFilesystemL();
	void HandleCommandL(TInt aCommand);
#if defined(__ER6__)
	virtual TKeyResponse CSidPlayAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
#else
	virtual void CSidPlayAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
#endif
	TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName, const TDesC8& /*aTail*/);
	void UpdateCbaL();

public:
	CSidPlayer* iPlayer;
	CSidPlayAppView* iAppView;
	};


//
// implementation of CSidPlayAppUi
//


void CSidPlayAppUi::ConstructL()
/**
 * safe constructor
 */
	{
	Dll::SetTls(this);

	BaseConstructL();

	//
	// create the player
	//
	_LIT(KSidTune, "c:\\sid\\gameover.sid");
	iPlayer = CSidPlayer::NewL(KSidTune);
	iPlayer->InitL();

	//
	// create the view
	//
	iAppView = new (ELeave) CSidPlayAppView();
	iAppView->ConstructL(ClientRect(), iPlayer);

#if !defined(__ER6__)
	// initialise toolband: set the font of the page movement buttons to the eikon symbolic font
	for (TInt i=ECmdPrev; i<=ECmdNext; i++)
		{
		CEikCommandButton* button = STATIC_CAST(CEikCommandButton*, iToolBand->ControlById(i));
		if(button)
			{
			CEikLabel* label = button->Label();
			label->SetFont(iEikonEnv->SymbolFont());
			}
		}

	// draw toolbars
	iToolBar->DrawNow();
	iToolBand->DrawNow();
#endif
	}


CSidPlayAppUi::~CSidPlayAppUi()
/**
 * D'tor
 */
	{
	if(iPlayer)
		delete iPlayer;
	delete iAppView;
	}


void CSidPlayAppUi::DoBrowseFileL()
/**
 * browse for .sid files
 */
	{
	ELOG1(_L8("CSidPlayAppUi::DoBrowseFileL\n"));

	TFileName path;
	_LIT(KDefaultDir, "C:\\sid\\");
	path.Append(KDefaultDir);

#if defined(__CRYSTAL__)
	const TBool success = CCknOpenFileDialog::RunDlgLD(path);
#elif defined(__SERIES60__)
	/*
	 * Stupid thing! cannot compile this because the file fullpathentry.h
	 * is missing from the SDK :(
	 */
//	CFileListDialog* list = CFileListDialog::NewL(path, KMyUid);
//	const TBool success = list->ExecuteLD() ;
	const TBool success = EFalse;
#elif defined(__QUARTZ__)
	/// @todo placeholder
	const TBool success = EFalse;
#else
	CDirContentsListBoxModel::TSortOrder order = CDirContentsListBoxModel::EOrderByName;
	CEikFileBrowserDialog* browseDialog = new (ELeave)
		CEikFileBrowserDialog(path, CEikFileBrowserDialog::EShowSystem, order);
	const TBool success = browseDialog->ExecuteLD(R_EIK_DIALOG_FILE_BROWSE);
#endif // __ER6__

	if(success)
		{
		// test
		iEikonEnv->InfoMsg(path);

		// out with the old one, in with the new one
		iPlayer->Stop();
		const TInt ret = iPlayer->NewTune(path);
		if(ret != KErrNone)
			{
			iEikonEnv->InfoMsg(_L("not good..."));
			}

		iPlayer->Play();
		UpdateCbaL();
		}
	}


void CSidPlayAppUi::DoScanFilesystemL()
/**
 * Scan the filesystem for .sid files.
 */
	{
	ELOG1(_L8("CSidPlayAppUi::DoScanFilesystemL\n"));

	_LIT(KSidExtension, "*.sid");
	_LIT(KRootDir, "\\");
	RFs& fs = Document()->Process()->FsSession();

	TFindFile file_finder(fs);
	CDir* file_list;
	TInt ret = file_finder.FindWildByDir(KSidExtension, KRootDir, file_list);
	while(ret==KErrNone)
		{
		for(TInt i=0; i<file_list->Count(); i++)
			{
			TParse fullentry;
			fullentry.Set((*file_list)[i].iName, &file_finder.File(), NULL);
			// Do something with the full EPOC filename
//			DoOneFile(aSession, fullentry.FullName());
			}
		delete file_list;
		ret = file_finder.FindWild(file_list);
		}
	}


void CSidPlayAppUi::HandleCommandL(TInt aCommand)
/**
 * handle key press from user
 *
 * @param aCommand Command to be handled
 */
	{
	ELOG2(_L8("CSidPlayAppUi::HandleCommandL [aCommand=0x%02x]\n"), aCommand);

	TInt delta = 0;
	TBuf<0x40> info_string;

	switch (aCommand)
		{
	case EEikCmdFileOpen:
		DoBrowseFileL();
		break;

	case EEikCmdExit:
		CBaActiveScheduler::Exit();
		break;

	case ECmdPrev:
		delta = -2;
		// fall through
	case ECmdNext:
		{
		delta += +1;
		const TInt song = iPlayer->SongDelta(delta);
		info_string.Format(_L("Song %d"), song);
		iEikonEnv->InfoMsg(info_string);
		iAppView->iStatusView->DrawNow();
		break;
		}
	case ECmdPlay:
		iPlayer->Play();
		iEikonEnv->InfoMsg(_L("play") );
		break;
	case ECmdPause:
		iPlayer->Pause();
		iEikonEnv->InfoMsg(_L("pause") );
		break;
	case ECmdStop:
		iPlayer->Stop();
		iEikonEnv->InfoMsg(_L("stop") );
		break;

	case ECmdAboutSidPlay:
		{
#if defined(__SERIES60__)
		  //	    CAknNoteDialog* dlg = new (ELeave) CAknNoteDialog();
		  //	    dlg->ExecuteDlgLD(R_SIDPLAY_ABOUT_NOTE);
#else
		TBuf<128> title;
		TBuf<512> about;
		TBuf<16> ver_epoc;
		ver_epoc.Copy(TPtrC8((TUint8*)epoc_version));
		TBuf<16> ver_base;
		ver_base.Copy(TPtrC8((TUint8*)emu_version));
		title.AppendFormat(_L("Sidplay %S for SymbianOS"), &ver_epoc);

		_LIT(KAbout, "ported by Alfred E. Heggestad\nhttp://esidplay.sourceforge.net/\nBaseversion is %S");
		about.Format(KAbout, &ver_base);
		(void)CCknInfoDialog::RunDlgLD(title, about);

#endif
		}
		break;

	case ECmdAboutSidTune:
		{
		sidTune& tune = iPlayer->CurrentSidTune();
		struct sidTuneInfo mySidInfo;
		tune.getInfo(mySidInfo);

		TBuf<256> buf0;
		buf0.Copy(TPtrC8((TUint8*)mySidInfo.nameString));

		TBuf<256> buf;
		buf.AppendFormat(_L("Load addr: 0x%04x\n"), mySidInfo.loadAddr);
		buf.AppendFormat(_L("Init addr: 0x%04x\n"), mySidInfo.initAddr);
		buf.AppendFormat(_L("Play addr: 0x%04x\n"), mySidInfo.playAddr);
		buf.AppendFormat(_L("C64 size:  0x%04x\n"), mySidInfo.c64dataLen);

#if defined(__CRYSTAL__)
		(void)CCknInfoDialog::RunDlgLD(buf0, buf);
#else
		// not yet
#endif
		}
		break;

	case ECmdAboutAudio:
		{
#if defined(__SERIES60__)
		  // not yet
#else
		struct emuConfig myEmuConfig;
		iPlayer->iEmuEngine->getConfig(myEmuConfig);

		_LIT(KAboutAudio, "Audio settings");

		TBuf<256> buf;
		buf.AppendFormat(_L("Frequency is %d Hz\n"), myEmuConfig.frequency);
		buf.AppendFormat(_L("Bits pr. sample is %d\n"), myEmuConfig.bitsPerSample);
		buf.AppendFormat(_L("Number of channels is %d\n"), myEmuConfig.channels);
		buf.AppendFormat(_L("SID filter is %S\n"), myEmuConfig.emulateFilter ?
			&_L("on") : &_L("off") );
		buf.AppendFormat(_L("Memory mode is "));
		if (myEmuConfig.memoryMode == MPU_PLAYSID_ENVIRONMENT)
			buf.AppendFormat(_L("PlaySID\n"));
		else if (myEmuConfig.memoryMode == MPU_TRANSPARENT_ROM)
			buf.AppendFormat(_L("transparent ROM\n"));
		else if (myEmuConfig.memoryMode == MPU_BANK_SWITCHING)
			buf.AppendFormat(_L("bank switching\n"));

		(void)CCknInfoDialog::RunDlgLD(KAboutAudio, buf);
#endif
		}
		break;

	case ECmdWavDump:
		{
		if(iPlayer->iAudio->IsWavDumping())
			{
			const TInt size = iPlayer->iAudio->StopWavDump();
			info_string.Format(_L("%d bytes dumped to %S.wav"), size, &iPlayer->iSongName);
			iEikonEnv->InfoMsg(info_string);
			}
		else
			{
			if(iPlayer->iSongName.Length())
				{
				const TInt ret = iPlayer->iAudio->StartWavDump(iPlayer->iSongName);
				if(ret)
					{
					info_string.Format(_L("Could not dump (%d)"), ret);
					iEikonEnv->InfoMsg(info_string);
					}
				else
					{
					info_string.Format(_L("Now dumping to %S.wav"), &iPlayer->iSongName);
					iEikonEnv->InfoMsg(info_string);
					}
				}
			}
		}
		break;

	default:
		info_string.Format(_L("cmd: %d"), aCommand);
		iEikonEnv->InfoMsg(info_string);
		break;
		}

	UpdateCbaL();
	}


#if defined(__ER6__)
TKeyResponse
#else
void
#endif
CSidPlayAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode /*aType*/)
/**
 * Handle a key event
 */
	{
	ELOG2(_L8("CSidPlayAppUi::HandleKeyEventL [code=%d]\n"), aKeyEvent.iCode);

	TInt delta = 0;
	TBuf<0x40> info_string;

	switch(aKeyEvent.iCode)
		{
	case EKeyUpArrow:
		{
		const TInt vol = iPlayer->VolumeDelta(+1);
		info_string.Format(_L("Volume++ (%d)"), vol);
		iEikonEnv->InfoMsg(info_string);
#if defined(__CRYSTAL__)
		iAppView->iVolumeView->SetAndDraw(vol);
#endif
		break;
		}

	case EKeyDownArrow:
		{
		const TInt vol = iPlayer->VolumeDelta(-1);
		info_string.AppendFormat(_L("Volume-- (%d)"), vol);
		iEikonEnv->InfoMsg(info_string);
#if defined(__CRYSTAL__)
		iAppView->iVolumeView->SetAndDraw(vol);
#endif
		break;
		}

	case EKeyLeftArrow:
		delta = -2;
		// fall through
	case EKeyRightArrow:
		{
		delta += +1;
		const TInt song = iPlayer->SongDelta(delta);
		info_string.AppendFormat(_L("Song %d"), song);
		iEikonEnv->InfoMsg(info_string);
		iAppView->iStatusView->DrawNow();
		break;
		}

		}

	//
	// pressing keys 0-9 gives the song, 0 is default
	//
	if(aKeyEvent.iCode >= '0' && aKeyEvent.iCode <= '9')
		{
		TBuf<0x40> string;
		TInt song = (TInt)(aKeyEvent.iCode - '0');
		const TInt ret = iPlayer->SongSelect(song);
		if(ret==KErrNone)
			{
			string.AppendFormat(_L("Song %d"), song);
			iEikonEnv->InfoMsg(string);
			}
		else
			{
			string.AppendFormat(_L("no song %d !"), song);
			iEikonEnv->InfoMsg(string);
			}
		iAppView->iStatusView->DrawNow();
		}

	UpdateCbaL();

#if defined(__ER6__)
	return EKeyWasConsumed;
#endif
	}


TBool CSidPlayAppUi::ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName, const TDesC8& /*aTail*/)
/**
 * called by EIKON framework during application startup. The variable aDocumentName contains the name of the file
 * that the user tapped on in shell.
 *
 * Some helpful info here:
 *
 * http://www2.epocworld.com/kbcppf.nsf/ce9a3a52e9d969788025670e00429dbd/df4f1b9d4146e6d3802569f3005eabf4?OpenDocument
 *
 * Headline:     Why does CMyAppUi::ProcessCommandParametersL() not seem to get called on V6 of the Symbian Platform?
 * Category:     Application architecture
 * EPOC Release: ER6
 * KeyWords:     Application architecture, Porting, ProcessCommandParametersL, Document, File names
 * Report:       Why does CMyAppUi::ProcessCommandParametersL() not seem to get called on V6 of the Symbian Platform?
 * Answer:
 *
 * This can often catch you out when porting code from ER5 to V6. The function:
 *
 * TBool CMyAppUi::ProcessCommandParametersL(TApaCommand aCommand,TFileName& aDocumentName,const TDesC&)
 *
 * on ER5 must be coded as:
 *
 * TBool CMyAppUi::ProcessCommandParametersL(TApaCommand aCommand,TFileName& aDocumentName,const TDesC8& )
 *
 * on V6 otherwise it will not get called.
 *
 */
	{
	TBuf8<256> buf8;
	buf8.Copy(aDocumentName);
	ELOG3(_L8("ProcessCommandParametersL [cmd=%d, doc=%S]\n"), aCommand, &buf8);

	if(aCommand==EApaCommandOpen && aDocumentName.Length())
		{
		// out with the old one, in with the new one
		iPlayer->Stop();
		const TInt ret = iPlayer->NewTune(aDocumentName);
		if(ret != KErrNone)
			{
			iEikonEnv->InfoMsg(_L("not good..."));
			}

		iPlayer->Play();
		UpdateCbaL();
		}

	// This prevents unnecessary zero length document files from being created
	aDocumentName.Zero();

/*
0	EApaCommandOpen,
1	EApaCommandCreate,
2	EApaCommandRun,
3	EApaCommandBackground,
*/
	return CEikAppUi::ProcessCommandParametersL(aCommand, aDocumentName);
	}


void CSidPlayAppUi::UpdateCbaL()
/**
 * Update the Command Button Array (CBA)
 */
	{
#if defined(__CRYSTAL__)
	CEikButtonGroupContainer* cba = iEikonEnv->AppUiFactory()->ToolBar();
	if(!cba)
		return;

	/*
	 * update play/stop buttons
	 */
	HBufC* text = NULL;
	const TInt play_stop_button = 2;
	if(iPlayer->iIdlePlay)
		{
		text = iEikonEnv->AllocReadResourceLC(R_SIDPLAY_STOP);
		cba->SetCommandL(play_stop_button, ECmdStop, *text);
		}
	else
		{
		text = iEikonEnv->AllocReadResourceLC(R_SIDPLAY_PLAY);
		cba->SetCommandL(play_stop_button, ECmdPlay, *text);
		}
	CleanupStack::PopAndDestroy(text);

	/*
	 * update prev/next buttons
	 */
	if(iPlayer)
		{
		struct sidTuneInfo mySidInfo;
		iPlayer->CurrentSidTune().getInfo( mySidInfo );

		const TBool dimmed = (mySidInfo.songs <= 1);

		cba->DimCommand(ECmdPrev, dimmed);
		cba->DimCommand(ECmdNext, dimmed);
		}

	cba->DrawNow();

#endif //__CRYSTAL__
	}


/**
 * CSidPlayDocument
 */
#if defined(__SERIES60__)
class CSidPlayDocument : public CAknDocument
#else
class CSidPlayDocument : public CEikDocument
#endif
	{
public:
	CSidPlayDocument(CEikApplication& aApp)
#if defined(__SERIES60__)
		:CAknDocument(aApp)
#else
		:CEikDocument(aApp)
#endif
	{
	}
private: // from CEikDocument
	CEikAppUi* CreateAppUiL();
	};


CEikAppUi* CSidPlayDocument::CreateAppUiL()
/**
 * create a new CEikAppUi object and return the pointer to it
 */
	{
	return new (ELeave) CSidPlayAppUi;
	}


/**
 * CSidPlayApplication - the actual application itself
 */
#if defined(__SERIES60__)
class CSidPlayApplication : public CAknApplication
#else
class CSidPlayApplication : public CEikApplication
#endif
	{
public:
	// from CEikApplication
//	virtual void GetDefaultDocumentFileName(TFileName& aDocumentName) const;
private: // from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};


TUid CSidPlayApplication::AppDllUid() const
/**
 * called by APPRUN
 */
	{
	return KMyUid;
	}


CApaDocument* CSidPlayApplication::CreateDocumentL()
/**
 * create a new document and return the pointer to it
 */
	{
	return new (ELeave) CSidPlayDocument(*this);
	}


void SidPlayExceptionHandler(TExcType aExc)
/**
 * our own exception handler to ease debugging
 */
	{
	const TInt back_trace = 4;

	_LIT(KTitle, "Sidplay internal error");
	TBuf<256> buf;
	buf.AppendFormat(_L("Exception code: %d\n"), aExc);

	CSidPlayAppUi* ui = (CSidPlayAppUi*)Dll::Tls();
	C6510* cpu = ui->iPlayer->iEmuEngine->iThe6510;
	TUint16 pc = cpu->abso();
	TUint8 op[back_trace];
	op[0] = (TUint8)cpu->my_read_data(pc);
	op[1] = (TUint8)cpu->my_read_data(pc-1);
	op[2] = (TUint8)cpu->my_read_data(pc-2);
	op[3] = (TUint8)cpu->my_read_data(pc-3);

	ELOG2(_L8("stackIsOkay=%d\n"), cpu->stackIsOkay);
	ELOG4(_L8("ac=0x%02x xr=0x%02x yr=0x%02x\n"), cpu->AC, cpu->XR, cpu->YR);
	ELOG3(_L8("pc=0x%04x sp=0x%04x\n"), cpu->PC, cpu->SP);
	ELOG2(_L8("pc=0x%04x opcodes:\n"), pc);
	for(TInt i = 0;i<back_trace;i++)
		ELOG3(_L8("0x%04x: 0x%02x\n"), pc-i, op[i]);

#if defined(__CRYSTAL__)
	(void)CCknInfoDialog::RunDlgLD(KTitle, buf);
#else
	CEikonEnv::InfoWinL(KTitle, buf);
//	CEikInfoDialog::RunDlgLD(KTitle, buf, CEikInfoDialog::EAllowEnter);
#endif
	}


//
// EXPORTed functions
//

EXPORT_C CApaApplication* NewApplication()
	{
	ELOG1(_L8("#\n"));
	ELOG2(_L8("# Sidplay for SymbianOS version %s\n"), epoc_version);
	ELOG2(_L8("# Based on libsidplay version %s\n"), emu_version);
	ELOG1(_L8("#\n"));
	ELOG2(_L8("# please go to %s for updates and more info.\n"), HOMEPAGE);
	ELOG1(_L8("# your bugreports and patches are welcome.\n"));
	ELOG2(_L8("# compiled on %s.\n"), __DATE__);
	ELOG1(_L8("#\n"));
	ELOG1(_L8("NewApplication: "));

/*
	TInt ret = RThread().SetExceptionHandler(SidPlayExceptionHandler, 0xffffffff); // 0xffffffff means handle all exc.
	if(ret)
		ELOG2(_L8("Could not install exception handler due to %d\n"), ret);
*/

	CApaApplication* p = new CSidPlayApplication;
	ELOG2(_L8("p=0x%08x\n"), p);
	return p;
	}


GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
/**
 * DLL entry point
 */
	{
	return KErrNone;
	}


// EOF - sidplayeik.cpp
