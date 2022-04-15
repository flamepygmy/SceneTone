/**
 * @file recsidfile.h Defines the class CRecSidFile for recognizing .SID files
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */
#ifndef RECSIDFILE_H__
#define RECSIDFILE_H__

#include <apadef.h>
#include <apaflrec.h>


class CRecSidFile : public CApaFileRecognizerType
	{
public:
	virtual TThreadId RunL(TApaCommand aCommand, const TDesC* aDocFileName=NULL,const TDesC8* aTailEnd=NULL) const;
private:
	TRecognizedType DoRecognizeFileL(RFs& aFs, TUidType aType);
	};

#endif // RECSIDFILE_H__
