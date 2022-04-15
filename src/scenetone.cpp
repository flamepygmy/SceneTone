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
#include <eikstart.h>
#include "scenetoneapplication.h"


LOCAL_C CApaApplication* NewApplication()
	{
	return new CScenetoneApplication;
	}

//#define SCENETONE_CRASH_LOGd

#if defined(SCENETONE_CRASH_LOG)

#include <utf.h>
#include <f32file.h>

RThread crashWatcherThread;
_LIT(KCrashWatcherName, "ScenetoneWatcher");
TThreadId debugThreadID;

_LIT (KDebugOutput, "c:\\scenetone.log");
_LIT8(KDebugFormat1, "Scenetone exception handler triggered.\nCategory: ");
_LIT8(KDebugFormat2, ", Reason: %d\nRegister dump:\n");
_LIT8(KDebugFormatRegDump, "R%d: 0x%x\n");

int crash_watcher_func(TAny *aStuff)
{
	// Open handle to the thread being watched
	RThread watchedThread;
	watchedThread.Open( debugThreadID );

	TRequestStatus  myreq   = KRequestPending;		
	TRequestStatus  *signal = (TRequestStatus*)aStuff;
	
	watchedThread.Logon( myreq );
	watchedThread.RequestComplete( signal, KErrNone);   // signal the main thread
	User::WaitForRequest( myreq );			 	        // start waiting for exit
	
	TInt 			  reason   = watchedThread.ExitReason();
	TExitCategoryName category = watchedThread.ExitCategory();
	TBuf8<64>		  regdumpd;
	watchedThread.Context(regdumpd);
	TUint*			  regdump  = (TUint*) regdumpd.Ptr();
	TBuf8<256> p;
	
	RFs rfs;
	rfs.Connect();
	RFile out;
	out.Replace(rfs, KDebugOutput, EFileWrite);
	
	TInt chars = CnvUtfConverter::ConvertFromUnicodeToUtf8(p, category);

	out.Write(KDebugFormat1);
	out.Write(p);
	p.Format(KDebugFormat2, reason); out.Write(p);

	for(int i=0;i<16;i++)
	{	
		p.Format(KDebugFormatRegDump, i, regdump[i] ); out.Write(p);
	}

	out.Close();
	rfs.Close();
	
	return KErrNone;
}

#endif

GLDEF_C TInt E32Main()
{
#if defined(SCENETONE_CRASH_LOG)
	RThread curthread;
	User::SetCritical( User::ENotCritical );

	TRequestStatus waitReady = KRequestPending;
	
	debugThreadID = curthread.Id();
	crashWatcherThread.Create( KCrashWatcherName, crash_watcher_func, 8192, 100000, 100000, (TAny*)&waitReady );
	crashWatcherThread.Resume( );

	// Wait for rendezvous
	User::WaitForRequest( waitReady );
	
#endif

	return EikStart::RunApplication( NewApplication );
}

