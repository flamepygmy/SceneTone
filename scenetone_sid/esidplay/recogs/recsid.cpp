/**
 * @file recsid.cpp Implements the class CRecSid for recognizing .SID files
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#include "recsid.h"

#ifdef _UNICODE
const TUid KUidRecSid = {0x10009a99};
#else
const TUid KUidRecSid = {0x10009a98};
#endif

const TInt KMinBufferLength =    5; // minimum amount of file needed to determine a SID file IF it's not called .sid
const TInt KMaxBufferLength = 1000; // maximum amount of buffer space we will ever use

_LIT8(KRecSidSignature, "PSID");
_LIT8(KDataTypeSid, "application/sid"); //TODO it this correct ?


//
// implementation of CRecSid
//


CRecSid::CRecSid()
/**
 * C'tor
 */
	:CApaDataRecognizerType(KUidRecSid, CApaDataRecognizerType::ELow)
	{
	iCountDataTypes = 1;
	}


TUint CRecSid::PreferredBufSize()
/**
 * returns the preferred buffer size
 */
	{
	return KMaxBufferLength;
	}


TDataType CRecSid::SupportedDataTypeL(TInt aIndex) const
/**
 * returns the supported data types
 */
	{
	__ASSERT_DEBUG(aIndex==0, User::Invariant());
	return TDataType(KDataTypeSid);
	}


void CRecSid::DoRecognizeL(const TDesC& /* aName */, const TDesC8& aBuffer)
/**
 * called by the Application Architecture Framework.
 *
 */
	{
	// check if file bigger than 5 bytes
	if (aBuffer.Length() < KMinBufferLength)
		return;

	// search for 'PSID'
	if (aBuffer.FindF(KRecSidSignature) >= 0)
		{
		// Yes it was found
		iConfidence = ECertain;
		iDataType = TDataType(KDataTypeSid);
		}
	}


EXPORT_C CApaDataRecognizerType* CreateRecognizer()
/**
 * The gate function - ordinal 1
 *
 * Create a new SID recogniser object (CRecSid) and return
 * the pointer to it.
 *
 * @return pointer to a newly created CRecSid object
 * @retval NULL out-of memory, no object created
 */
	{
	return new CRecSid();
	}


GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
/**
 * DLL entry point
 */
	{
	return KErrNone;
	}

// EOF - recsid.cpp
