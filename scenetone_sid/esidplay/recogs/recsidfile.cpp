/**
 * @file recsidfile.cpp Implements the class CRecSidFile for recognizing .SID files
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#include "recsidfile.h"
#include <e32svr.h>
#include <s32file.h>
#include <f32file.h>
#include <apaid.h>
#include <apacmdln.h>
#include <apgcli.h>


/**
 * This UID specifies the UID of the SIDPLAY application
 * which bill started when user clicks on a .SID file
 * These UIDs are defined in sidplay.mmp
 */
#ifdef _UNICODE
const TUid KUidSidPlayer = {0x10009a97}; // unicode
#else
const TUid KUidSidPlayer = {0x10009a96}; // narrow
#endif

_LIT8(KRecSidSignature, "PSID");


CApaFileRecognizerType::TRecognizedType CRecSidFile::DoRecognizeFileL(RFs& aFs, TUidType /*aType*/)
/**
 * Recognises SID files - by examining the file's contents
 */
	{
	iRecognizedType = ENotRecognized;

	// SID files are recognized by contents, not by extension!
	RFile file;
	if(file.Open(aFs, *iFullFileName, EFileShareReadersOnly) != KErrNone)
		return iRecognizedType;
	TBuf8<1024> buf;
	TInt err = file.Read(buf);
	file.Close();
	if (err!=KErrNone)
		return iRecognizedType;
	if (buf.FindF(KRecSidSignature) != KErrNotFound)
		{
		iRecognizedType = EDoc;
		iAppUid = KUidSidPlayer;
		return iRecognizedType;
		}
	return iRecognizedType;
	}


TThreadId CRecSidFile::RunL(TApaCommand aCommand, const TDesC* aDocFileName, const TDesC8* aTailEnd) const
	{
	CApaCommandLine* commandLine = CApaCommandLine::NewLC();
	commandLine->SetCommandL(aCommand);
	if(iRecognizedType == EDoc)
		{
		// We've recognised a doc so we need to scan for its associated app
		TApaAppEntry appEntry;
		User::LeaveIfError(iFileRecognizer->AppLocator()->GetAppEntryByUid(appEntry,iAppUid));
		commandLine->SetLibraryNameL(appEntry.iFullName);
		if(aDocFileName)
			commandLine->SetDocumentNameL(*iFullFileName);
		}
	else
		User::Leave(KErrNotSupported);

//	if (aTailEnd)
//		commandLine->SetTailEndL(*aTailEnd);
	TThreadId id = AppRunL(*commandLine);
	CleanupStack::PopAndDestroy(); // commandLine
	return id;
	}


EXPORT_C CApaFileRecognizerType* CreateRecognizer()
/**
 * Create a new SID recogniser object (CRecSid) and return
 * the pointer to it. Called by AppArc
 *
 * @return poiner to newly created CRecSid object
 * @retval NULL out-of memory, no object created
 */
	{
	return new CRecSidFile();
	}


GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
/**
 * DLL entry point
 */
	{
	return KErrNone;
	}


// EOF - recsidfile.cpp
