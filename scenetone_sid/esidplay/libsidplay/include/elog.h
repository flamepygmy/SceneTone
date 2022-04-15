// ELOG.H
//
// (c) 2000-2002 Alfred E. Heggestad
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License version 2 as
//    published by the Free Software Foundation.
//
#if !defined(ELOG_H__)
#define ELOG_H__

/**
 * @file elog.h
 *
 * defines the interface to file logging for EPOC programs
 */


//
// normal debugging
//
#if defined(_DEBUG) || defined(__WINS__)

#ifndef __E32DEF_H__
  #ifdef NULL
  #undef NULL
  #endif
#endif

#include <f32file.h>
#define ELOG(a)           a
#define ELOG1(a)          CEpocLogger::DoLog(a)
#define ELOG2(a, b)       CEpocLogger::DoLog(a, (b) )
#define ELOG3(a, b, c)    CEpocLogger::DoLog(a, b, c)
#define ELOG4(a, b, c, d) CEpocLogger::DoLog(a, b, c, d)

class CEpocLogger : public CBase
	{
  public:
	static void DoLog(TRefByValue<const TDesC8> aFmt, ...);
	};

#else // no debugging

#define ELOG(a)
#define ELOG1(a)
#define ELOG2(a, b)
#define ELOG3(a, b, c)
#define ELOG4(a, b, c, d)

#endif


#endif  // ELOG_H__
