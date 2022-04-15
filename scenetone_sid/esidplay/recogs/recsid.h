/**
 * @file recsid.h Defines the class CRecSid for recognizing .SID files
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */
#ifndef RECSID_H__
#define RECSID_H__


#include <apmrec.h>

/**
 * Realises a CApaDataRecognizerType class for
 * recognizing .SID files on an EPOC device.
 */
class CRecSid : public CApaDataRecognizerType
	{
public:
	CRecSid();

// -- CApaDataRecognizerType
public:
	TUint PreferredBufSize();
	TDataType SupportedDataTypeL(TInt aIndex) const;
private:
	void DoRecognizeL(const TDesC& aName, const TDesC8& aBuffer);
	};

#endif // RECSID_H__
