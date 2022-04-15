/**
 * @file esidplay.cpp Implements a simple text shell SidPlayer using the
 * real libsidplay.dll library. Can be replaced by the Eikon version with a
 * better UI. Based on sidplay.cpp by Michael Scwendt from the original SidPlay
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#if defined(__ER6__)
#include "audiodrv_er6.h"
#else
#include "audiodrv_epoc.h"
#endif

#include <e32test.h>
#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "myendian.h"

#include "6510_.h"

GLDEF_D RTest gTest(_L("esidplay"));

const TInt EXIT_ERROR_STATUS = (-1);

//
// global variables
//
LOCAL_D emuEngine*   myEmuEngine;
LOCAL_D TInt16*       buffer;
LOCAL_D audioDriver* myAudio;
LOCAL_D sidTune*     myTune;
LOCAL_D int          bufSize;
LOCAL_D uword selectedSong = 0; // default song

ELOG( LOCAL_D int count = 0;)

const uword fragments = 16;
const uword fragSizeBase = 12;

//
// forward declarations
//
GLREF_C bool sidEmuInitializeSong(emuEngine & thisEmuEngine,
								  sidTune & thisTune,
								  uword songNumber);


TInt SidPlayerThread(TAny* )
/**
 * this is the Sid player idle thread.
 *
 */
	{
	gTest.Printf(_L("."));
	ELOG2(_L8("count=%d \n"), count++);

	if(!myAudio->iIsReady)
		return ETrue;

	if(!myAudio->iPrefilled)
		{
		myEmuEngine->FillBuffer(*myTune, buffer, bufSize);
		gTest.Printf(_L("/"));
		myAudio->iPrefilled = ETrue;
		}

	if(myAudio->iPrefilled && myAudio->iBlocksInQueue == 0)
		{
		myAudio->Play((ubyte*)buffer, bufSize);
		gTest.Printf(_L("-"));
		myAudio->iBlocksInQueue = 1;
		myAudio->iPrefilled = EFalse;
		}

	return ETrue;
	}

//
// ---
//


void SidPlayExceptionHandler(TExcType aExc)
	{
	gTest.Printf(_L("### SOMETHING went wrong (exception = %d)\n"), aExc);

	ELOG2(_L8("dump: myEmuEngine: 0x%08x \n") , myEmuEngine );
	ELOG2(_L8("      buffer     : 0x%08x \n") , buffer );
	ELOG2(_L8("      myAudio    : 0x%08x \n") , myAudio );
	ELOG2(_L8("      myTune     : 0x%08x \n") , myTune );

	/*
	emuEngine* ee = (emuEngine*)Dll::Tls();
	C6510* cpu = ee->iThe6510;
	TUint16 pc = cpu->abso();
	TUint8 op  = cpu->my_read_data(pc);

	ELOG3(_L8("pc=0x%04x opcode=0x%02x\n"), pc, op);
	gTest.Printf(_L("pc=0x%04x opcode=0x%02x\n"), pc, op);
	*/

	FOREVER
		  {
		  TInt ret = getchar();
		  if(ret == 'q' || ret == 'Q')
			  User::Panic(_L("bye"), 0);
		  }
	}


// Error and status message numbers.
enum
{
	ERR_NOT_ENOUGH_MEMORY,
	ERR_SYNTAX,
	ERR_ENDIANESS
};

void printtext( int messageNum );

static bool verboseOutput = false;


/**
 * this class reads characters asyncronously from the console.
 *
 * it is used for e.g. getting the Ctrl-C key when playing SID tunes.
 */
class CConsoleReader : public CActive
	{
public:
	CConsoleReader(CConsoleBase& aConsole);
	~CConsoleReader();
protected:
	void Activate();
// from CActive
	virtual void DoCancel();
	virtual void RunL();
private:
	TBuf8<2> buffTx;
	CConsoleBase& iConsole;
	};

CConsoleReader::CConsoleReader(CConsoleBase& aConsole)
/**
 * C'tor
 *
 * @param aConsole reference to the console to read from
 */
	:CActive(EPriorityHigh)
	,buffTx(2)
	,iConsole(aConsole)
	{
	CTOR(CConsoleReader);

	iStatus = 0;

	CActiveScheduler::Add(this);
	Activate();
	}

CConsoleReader::~CConsoleReader()
/**
 * D'tor
 */
	{
	DTOR(CConsoleReader);
	if(IsAdded())
		Deque(); /// @todo necessary ?

	DoCancel(); /// @todo picked up by Lint. Is it necessary ?
	}

void CConsoleReader::Activate()
/**
 * read from the console and activate the active object
 */
	{
	ELOG1(_L8("CConsoleReader::Activate\n") );
	iConsole.Read(iStatus);
	CActive::SetActive();
	}

void CConsoleReader::DoCancel()
/**
 * cancel any outstanding events
 */
	{
	iConsole.ReadCancel();
	}

void CConsoleReader::RunL()
/**
 * asyncronous event received. A key was pressed. In case of Ctrl-C, exit.
 * if not, re-issue the read and go back to the scheduler.
 */
	{
	ELOG2(_L8("CConsoleReader::RunL iStatus=%d\n"), iStatus.Int() );

	const TInt key = iConsole.KeyCode();
	switch(key)
		{
	case 'q':
	case 'Q':
	case 0x03: // Ctrl-C
		CActiveScheduler::Stop();
		return; // do not activate

	case EKeyUpArrow:
		{
		const TInt vol = myAudio->VolumeDelta(+1);
		gTest.Printf(_L("\nVolume++ (%d)"), vol);
		break;
		}

	case EKeyDownArrow:
		{
		const TInt vol = myAudio->VolumeDelta(-1);
		gTest.Printf(_L("\nVolume-- (%d)"), vol);
		break;
		}

	case EKeyLeftArrow:
		selectedSong-=2;
		// fall through
	case EKeyRightArrow:
		++selectedSong;
		if ( !sidEmuInitializeSong(*myEmuEngine, *myTune, selectedSong) )
			{
			gTest.Printf(_L("SIDPLAY: SID Emulator Engine components not ready"));
			}
		gTest.Printf(_L("\nsong: %d"), selectedSong);
		break;


#ifdef __PROFILING__
	case 'p':
	case 'P':
		{
		TFixedArray<TProfile, EProfiles> result;
		RDebug::ProfileResult(result.Begin(), 0, EProfiles);
		gTest.Printf(_L("===dumping profiles===\n" ));
		ELOG1(_L8("===dumping profiles===\n") );
		for (TInt i=0; i<EProfiles; i++)
			{
			gTest.Printf(_L("Profile %d:  Calls: %d, Clock ticks: %d Avg: %d\n"),
						 i, result[i].iCount, result[i].iTime,
						 result[i].iCount ? (result[i].iTime / result[i].iCount) : -1 );
			TBuf8<256> buf;
			buf.AppendFormat(_L8("Profile %d:  Calls: %d, Clock ticks: %d Avg: %d\n"),
							 i, result[i].iCount, result[i].iTime,
							 result[i].iCount ? (result[i].iTime / result[i].iCount) : -1 );
			ELOG1(buf);
			}
		}

#endif // __PROFILING__


	  default:
		  gTest.Printf(_L("%c [0x%04x]"), key, key);
		  break;
		}
	Activate(); // kick it
	}


LOCAL_C void InitMainL(void)
/**
 * initialise the emulation engine and start the SID chip emulator
 */
	{
	TInt ret;

	ELOG1(_L8("----------new log----------\n") );

// --------------------------------------------------

#if 0
	// installing exception handler
	_ELOG(_L8("installing exception handler...") );
	ret = RThread().SetExceptionHandler(SidPlayExceptionHandler, 0xffffffff); // 0xffffffff means handle all exc.
	if(ret == KErrNone)
		_ELOG(_L8("OK\n"));
	else
		_ELOG(_L8("failed!\n"));
#endif

	gTest.Printf(_L("\nSIDPLAY   Music player and C64 SID chip emulator   Base Version %s \n"), emu_version );
	gTest.Printf(_L("Copyright (c) 1994-1997 Michael Schwendt   All rights reserved.\n"));
	gTest.Printf(_L("Ported to Epoc32 by Alfred E. Heggestad <edmund@roland.org>\n"));
	gTest.Printf(_L("This is version %s\n"), epoc_version);
	gTest.Printf(_L("\n"));
	gTest.Printf(_L("Use arrow keys for volume and song number\n"));

	// ======================================================================
	// INITIALIZE THE EMULATOR ENGINE
	// ======================================================================

	ELOG1(_L8("initialising EE (Emulator Engine)...\n"));
	myEmuEngine = new (ELeave) emuEngine();
	ELOG1(_L8("emuEngine created\n"));

	//	Dll::SetTls(myEmuEngine);

	// Initialize the SID-Emulator Engine to defaults.
	if ( !myEmuEngine->verifyEndianess() )
	{
		printtext(ERR_ENDIANESS);
	}

	ELOG1(_L8("...Done\n"));

	// Get the default configuration.
	struct emuConfig myEmuConfig;
	myEmuEngine->getConfig(myEmuConfig);

	// ======================================================================

	// at least one argument required
#if defined(__ER6__)
	if (RProcess().CommandLineLength() == 0)
		printtext(ERR_SYNTAX);
	TBuf<256> cmdLineBuf;
	RProcess().CommandLine(cmdLineBuf);
#else
	TCommand cmdLineBuf = RProcess().CommandLine();
	if (cmdLineBuf.Length() == 0)
		printtext(ERR_SYNTAX);
#endif

	// Default audio settings.
#ifdef __ER6__
	myEmuConfig.frequency = SAMPLE_FREQ;
	myEmuConfig.channels = SIDEMU_MONO;
	myEmuConfig.bitsPerSample = SIDEMU_16BIT;
#else
	myEmuConfig.frequency = KAlawSamplesPerSecond;
	myEmuConfig.channels = SIDEMU_MONO;
	myEmuConfig.bitsPerSample = SIDEMU_8BIT;
#endif

	ELOG1(_L8("parsing command line arguments..."));

	// parse command line arguments
/*
	int pos = 0;
	char* argv = &cmdLineBuf[0];

	while (pos < cmdLineBuf.Length())
	{

		if ( argv[0] == '-')
		{
			pos++;
			switch ( argv[1] )
			{
			 case 'c':
				myEmuConfig.forceSongSpeed = true;
				pos++;
				break;
			 case 'a':
				if ( argv[2] == '2' )
					myEmuConfig.memoryMode = MPU_BANK_SWITCHING;
				else
					myEmuConfig.memoryMode = MPU_PLAYSID_ENVIRONMENT;
				pos+=2;
				break;
			 case 'f':
				myEmuConfig.frequency = (udword)atol(argv+2);
				break;
			 case 'h':
				printtext(ERR_SYNTAX);
				break;
			 case 'n':
				if ( argv[2] == 'f' )
					myEmuConfig.emulateFilter = false;
				else
					myEmuConfig.clockSpeed = SIDTUNE_CLOCK_NTSC;
				break;
			 case 'o':
				selectedSong = atoi(argv+2);
				break;
			 case 'p':
				if ( argv[2] == 'c' )
				{
					myEmuConfig.autoPanning = SIDEMU_CENTEREDAUTOPANNING;
				}
				break;
			 case 's':
				myEmuConfig.channels = SIDEMU_STEREO;
				if ( argv[2] == 's' )
				{
					myEmuConfig.volumeControl = SIDEMU_STEREOSURROUND;
				}
				break;
			 case 'v':
				verboseOutput = true;
				pos++;
				break;
			 case ' ':
				pos++;
				break;
			 default:
				printtext(ERR_SYNTAX);
				break;
			}
		}
		else
		{
			if ( infile == 0 )
				infile = a;  // filename argument
			else
				printtext(ERR_SYNTAX);
		}
//		a++;  // next argument
	};

	ELOG1(_L8("Done\n"));

	if (infile == 0)
	{
		printtext(ERR_SYNTAX);
	}
*/

	// ======================================================================
	// VALIDATE SID EMULATOR SETTINGS
	// ======================================================================

	ELOG1(_L8("Validating SID emulator settings..."));

	if ((myEmuConfig.autoPanning!=SIDEMU_NONE) && (myEmuConfig.channels==SIDEMU_MONO))
	{
		myEmuConfig.channels = SIDEMU_STEREO;  // sane
	}
	if ((myEmuConfig.autoPanning!=SIDEMU_NONE) && (myEmuConfig.volumeControl==SIDEMU_NONE))
	{
		myEmuConfig.volumeControl = SIDEMU_FULLPANNING;  // working
	}

	ELOG1(_L8("Done\n"));

	// ======================================================================
	// INSTANTIATE A SIDTUNE OBJECT
	// ======================================================================

	ELOG1(_L8("creating Sidtune object..."));
	{
	TBuf8<256> buf8;
	buf8.Copy(cmdLineBuf);
	myTune = new sidTune((const char*)buf8.PtrZ());
	}
	struct sidTuneInfo mySidInfo;
	myTune->getInfo( mySidInfo );
	if ( !myTune )
	{
		gTest.Printf(_L("SIDPLAY: %s\n"), mySidInfo.statusString);
		exit(EXIT_ERROR_STATUS);
	}
	else
	{
		if (verboseOutput)
		{
//		gTest.Printf(_L("File format  : %s \n"), mySidInfo.formatString);
//		gTest.Printf(_L("Filenames    : %s , %s \n"), mySidInfo.dataFileName, mySidInfo.infoFileName );
//		gTest.Printf(_L("Condition    : %s \n"), mySidInfo.statusString );
		}
		gTest.Printf(_L("--------------------------------------------------\n"));
		if ( mySidInfo.numberOfInfoStrings == 3 )
		{
//			gTest.Printf(_L("Name         : %s \n"), mySidInfo.nameString);
//			gTest.Printf(_L("Author       : %s \n"), mySidInfo.authorString);
//			gTest.Printf(_L("Copyright    : %s \n"), mySidInfo.copyrightString);

		}
		else
		{
			for ( int infoi = 0; infoi < mySidInfo.numberOfInfoStrings; infoi++ )
				gTest.Printf(_L("Description  : %s\n"), mySidInfo.infoString[infoi] );
		}
		gTest.Printf(_L("--------------------------------------------------\n"));
		if (verboseOutput)
		{
			gTest.Printf(_L("Load address : $%04x\n"), mySidInfo.loadAddr );
			gTest.Printf(_L("Init address : $%04x\n"), mySidInfo.initAddr );
			gTest.Printf(_L("Play address : $%04x\n"), mySidInfo.playAddr );
		}
	}

	ELOG1(_L8("Done\n"));

	// ======================================================================
	// CONFIGURE THE AUDIO DRIVER
	// ======================================================================

#if defined (__WINS__)
#define PDD_NAME _L("ESDRV")
#define LDD_NAME _L("ESOUND")

	gTest.Printf(_L("loading device drivers...(WINS only)"));

	ret = User::LoadPhysicalDevice(PDD_NAME);
	if (ret!=KErrNone && ret!=KErrAlreadyExists)
		User::LeaveIfError(ret);

	ret = User::LoadLogicalDevice(LDD_NAME);
	if (ret!=KErrNone && ret!=KErrAlreadyExists)
		User::LeaveIfError(ret);

	gTest.Printf(_L("Loaded!\n"));
#endif


	ELOG1(_L8("check if audio device exist..."));

	// Instantiate the audio driver. The capabilities of the audio driver
	// can override the settings of the SID emulator.
	myAudio = new (ELeave) audioDriver();
	myAudio->ConstructL();

	if ( !myAudio->IsThere() )
	{
		gTest.Printf(_L("SIDPLAY: No audio device available !\n"));
		exit(EXIT_ERROR_STATUS);
	}
	ELOG1(_L8("Done\n"));

	// Open() does not accept the "bitsize" value on all platforms, e.g.
	// Sparcstations 5 and 10 tend to be 16-bit only at rates above 8000 Hz.
	ELOG1(_L8("opening sound device... "));
	if ( !myAudio->Open(myEmuConfig.frequency, myEmuConfig.bitsPerSample,
					   myEmuConfig.channels, fragments, fragSizeBase))
	{
	    gTest.Printf(_L("%s \n"), myAudio->GetErrorString() );
		exit(EXIT_ERROR_STATUS);
	}
	ELOG1(_L8("Done\n"));


	if (verboseOutput)
	{
		gTest.Printf(_L("Block size   : %d \n"), (udword)myAudio->GetBlockSize() );
		gTest.Printf(_L("Fragments    : %d \n"), myAudio->GetFragments() );
	}

	// ======================================================================
	// CONFIGURE THE EMULATOR ENGINE
	// ======================================================================

	ELOG1(_L8("Configuring the Emulator Engine..."));

	// Configure the SID emulator according to the audio driver settings.
	myEmuConfig.frequency = (uword)myAudio->GetFrequency();
	myEmuConfig.bitsPerSample = myAudio->GetSamplePrecision();
	myEmuConfig.sampleFormat = myAudio->GetSampleEncoding();
	ret = myEmuEngine->setConfig( myEmuConfig );
	if(!ret)
		{
	    gTest.Printf(_L("Could not SetConfig on emu engine\n") );
		exit(EXIT_ERROR_STATUS);
		}

	// Print the relevant settings.
	if (verboseOutput)
	{
	    gTest.Printf(_L("Frequency    : %d Hz\n"), myEmuConfig.frequency );
		gTest.Printf(_L("SID Filter   : %s \n"), (myEmuConfig.emulateFilter ? "Yes" : "No") );
		if (myEmuConfig.memoryMode == MPU_PLAYSID_ENVIRONMENT)
		{
		gTest.Printf(_L("Memory mode  : PlaySID (this is supposed to fix PlaySID-specific rips)\n"));
		}
		else if (myEmuConfig.memoryMode == MPU_TRANSPARENT_ROM)
		{
		gTest.Printf(_L("Memory mode  : Transparent ROM (SIDPLAY default)\n"));
		}
		else if (myEmuConfig.memoryMode == MPU_BANK_SWITCHING)
		{
		gTest.Printf(_L("Memory mode  : Bank Switching\n"));
		}
	}
	ELOG1(_L8("Done\n"));

	// ======================================================================
	// INITIALIZE THE EMULATOR ENGINE TO PREPARE PLAYING A SIDTUNE
	// ======================================================================

	ELOG1(_L8("preparing to play a Sidtune..."));
	if ( !sidEmuInitializeSong(*myEmuEngine, *myTune, selectedSong) )
	{
	    gTest.Printf(_L("SIDPLAY: SID Emulator Engine components not ready"));
		exit(EXIT_ERROR_STATUS);
	}
	ELOG1(_L8("Done\n"));

	// Read out the current settings of the sidtune.
	ELOG1(_L8("Reading current setting of the Sidtune..."));
	myTune->getInfo( mySidInfo );
	if ( !myTune )
		{
		gTest.Printf(_L("SIDPLAY: %s \n"), mySidInfo.statusString );
		exit(EXIT_ERROR_STATUS);
		}
	ELOG1(_L8("Done\n"));

	gTest.Printf(_L("Setting song : %d out of %d (default = %d)"),
				 mySidInfo.currentSong, mySidInfo.songs, mySidInfo.startSong );
	selectedSong = mySidInfo.startSong;
	if (verboseOutput)
		{
		gTest.Printf(_L("Song speed   : %s \n"), mySidInfo.speedString );
		}

	// ======================================================================
	// KEEP UP A CONTINUOUS OUTPUT SAMPLE STREAM
	// ======================================================================

	ELOG1(_L8("keep up a continuous output sample stream..."));
	bufSize = myAudio->GetBlockSize();

	if(bufSize <= 0 )
		{
		gTest.Printf(_L("Error: buffer size is not positive! (%d) \n"), bufSize);
		exit(EXIT_ERROR_STATUS);
		}

	if ((buffer = new TInt16[bufSize]) == 0)
		{
		printtext(ERR_NOT_ENOUGH_MEMORY);
	    exit(EXIT_ERROR_STATUS);
		}

	ELOG1(_L8("Done\n"));

	ELOG3(_L8("BUF: size = %d at 0x%08x\n"), bufSize, buffer );


	gTest.Printf(_L("Playing, press Ctrl-C to stop ...\n"));
	ELOG1(_L8("now playing...\n"));


	//
	// create a new Console reader and the CIdle active object
	// the console reader is waiting for key input while
	// the idle callback function using all the CPU cycles
	// for good old 6581 & 6510 chip emulation...!
	//
	CConsoleReader* cons = new (ELeave) CConsoleReader(*gTest.Console());

	ELOG1(_L8("Creating new CIdle object. \n"));
	CIdle* idle = CIdle::NewL(CActive::EPriorityLow);
	idle->Start(TCallBack(SidPlayerThread));
	ELOG1(_L8("Callback installed. \n"));

	gTest.Printf(_L("SID emulation started\n"));
	CActiveScheduler::Start();
	gTest.Printf(_L("SID emulation stopped\n"));

#ifndef __ER6__
	delete idle;
#endif
	delete cons;

	ELOG1(_L8("stopped! \n"));

	//
	// cleanup
	//
	delete myEmuEngine;
	delete myAudio;
	delete myTune;
	delete buffer;

	exit(0);
	}


void printtext(int number)
{
  switch (number)
	{
	 case ERR_ENDIANESS:
		{
		    gTest.Printf(_L("SIDPLAY: ERROR: Hardware endianess improperly configured.\n"));
			exit(EXIT_ERROR_STATUS);
			break;
		}
	 case ERR_NOT_ENOUGH_MEMORY:
		 gTest.Printf(_L("SIDPLAY: ERROR: Not enough memory\n"));
		exit(EXIT_ERROR_STATUS);
	 case ERR_SYNTAX:
		 gTest.Printf(_L(" syntax: sidplay [-<command>] <datafile>|-\n"));
		 gTest.Printf(_L(" commands: -h       display this screen\n"));
		 gTest.Printf(_L("           -v       verbose output\n"));
		 gTest.Printf(_L("           -f<num>  set frequency in Hz (default: 22050)\n"));
		 gTest.Printf(_L("           -o<num>  set song number (default: preset)\n"));
		 gTest.Printf(_L("           -a       improve PlaySID compatibility (read the docs !)\n"));
		 gTest.Printf(_L("           -a2      bank switching mode (overrides -a)\n"));
#if defined(linux) || defined(__FreeBSD__) || defined(__amigaos__) || defined(hpux)
		 gTest.Printf(_L("           -s       enable stereo replay\n"));
		 gTest.Printf(_L("           -ss      enable stereo surround\n"));
		 gTest.Printf(_L("           -pc      enable centered auto-panning (stereo only)\n"));
#endif
		 gTest.Printf(_L("           -nf      no SID filter emulation\n"));
		 gTest.Printf(_L("           -n       set NTSC clock speed (default: PAL)\n"));
		 gTest.Printf(_L("           -c       force song speed = clock speed (PAL/NTSC)\n"));
		 gTest.Printf(_L("           -bn<num> set number of audio buffer fragments to use\n"));
		 gTest.Printf(_L("           -bs<num> set size 2^<num> of audio buffer fragments\n"));
		(void)gTest.Getch();
		exit(EXIT_ERROR_STATUS);
	 default:
		 gTest.Printf(_L("SIDPLAY: ERROR: Internal system error\n"));
		exit(EXIT_ERROR_STATUS);
	}

}


LOCAL_C void RunTestsL()
/**
 * Run all the tests
 */
	{
	InitMainL();
	}


GLDEF_C TInt E32Main()
/**
 * Main
 */
	{

	// get number of Handles *before* we start the program
	TInt processHandleCountBefore;
	TInt threadHandleCountBefore;
	RThread().HandleCount(processHandleCountBefore, threadHandleCountBefore);

	CTrapCleanup* cleanup = CTrapCleanup::New();

	__UHEAP_MARK;

	CActiveScheduler* theActiveScheduler = new CActiveScheduler();
	CActiveScheduler::Install(theActiveScheduler);

	gTest.Printf(_L("."));

	TRAPD(err, RunTestsL());
	if (err!=KErrNone)
		{
		gTest.Printf(_L("ERROR: Leave %d - press a key\n"), err);
		(void)gTest.Getch(); // ignore return value
		}

	gTest.End();
	gTest.Close();

	__UHEAP_MARKEND;

	__ASSERT_ALWAYS(RThread().RequestCount()==0, User::Panic(_L("outstanding requests!"), RThread().RequestCount()) );

	// get number of Handles *after* the program is finished
	TInt processHandleCountAfter;
	TInt threadHandleCountAfter;
	RThread().HandleCount(processHandleCountAfter, threadHandleCountAfter);

	__ASSERT_ALWAYS(threadHandleCountBefore==threadHandleCountAfter,
					User::Panic(_L("outstanding handles!"), threadHandleCountAfter) );

	delete cleanup;
	return KErrNone;
	}


//
// some extra Epoc stuff here
//

GLDEF_C TInt E32Dll(TDllReason)
/**
 * DLL entry point
 */
	{
	return KErrNone;
	}


// EOF - ESIDPLAY.CPP
