/**
 * @file elog.cpp Implements file logging for EPOC programs.
 *
 * Copyright (c) 2000-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#include "elog.h"
#include <e32test.h>

#if defined (_DEBUG) || defined(__WINS__)

/**
 * Class to handle overflow.
 */
class TLoggerOverflow : public TDes8Overflow
	{
public:
	void Overflow(TDes8& /*aDes*/) {}
	};


//
// local variables
//
_LIT(KTheLogFile, "c:\\sidplaylog.txt");
const TInt KFileOpenWritable = EFileWrite | EFileShareAny;


void CEpocLogger::DoLog(TRefByValue<const TDesC8> aFmt, ...)
/**
 * append text to the log file.
 *
 * create a new one if it does not exist
 */
	{
	// get the Variable Argument list (...)
	VA_LIST list;
	VA_START(list,aFmt);

	TLoggerOverflow logOverflow;
	TBuf8<256> logBuf;
	logBuf.AppendFormatList(aFmt,list,&logOverflow);

	TInt ret;
	RFs theLogFs;
	RFile theLogFile;

	ret = theLogFs.Connect();
	if( (ret != KErrNone) && (ret != KErrAlreadyExists) )
		return;

	// try to open to old one first
	ret = theLogFile.Open(theLogFs, KTheLogFile, KFileOpenWritable);
	if(ret == KErrNotFound)
		ret = theLogFile.Replace(theLogFs, KTheLogFile, KFileOpenWritable);
	if(ret != KErrNone)
		return;

	TInt pos;
	ret = theLogFile.Seek(ESeekEnd, pos);
	if (ret==KErrNone)
		theLogFile.Write(logBuf);

	theLogFile.Close(); // close it so that 'tail -f logfile.txt' can display changes
	theLogFs.Close();
	}


#endif // _DEBUG
