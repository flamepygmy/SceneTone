// ETOOLS.H
//
// Copyright (c) 1999 A. Heggestad. Conditions dont apply
//

#if !defined(__ETOOLS_H__)
#define __ETOOLS_H__

#if defined (__EPOC32__)
#include <e32std.h>
#endif

// Tool for putting a name to the class
// when a memory leak occurs, you can trace it back
// by looking at the memory space!
// the for(;;) is a bit awkward
#if _DEBUG
#define CLASSNAMEDECL(a) char __iName[sizeof(#a)];
#define CLASSNAMEINIT(a) \
	char tmp[] = #a; \
	for(TUint i=0;i<sizeof(tmp);i++) __iName[i] = tmp[i];
#else
#define CLASSNAMEDECL(a)
#define CLASSNAMEINIT(a)
#endif


//
// put this in your constructor/desctructor to see the memory usage
//
#if defined(_DEBUG) || defined(__WINS__)
#define CTOR(a)	ELOG4(_L8("created object %s (%d bytes) at 0x%08x\n"), #a, sizeof(a), this );
#define DTOR(a)	ELOG4(_L8("deleted object %s (%d bytes) at 0x%08x\n"), #a, sizeof(a), this );
#else
#define CTOR(a)
#define DTOR(a)
#endif

//
// Use this macro in the beginning of functions to check that some ptrs are != NULL
//

#ifndef __PANIC
#ifdef _UNICODE
#define __PANIC(p) { char tmp[] = __FILE__; \
		TInt i;\
		for(i=sizeof(tmp)-1;i>=0;i--) \
			if( (tmp[i] == 0x5c /* '\' */) || (tmp[i] == '/') ) \
				break; \
		TBuf8<32> txt8((const TUint8*)&tmp[++i]); \
		TBuf16<32> txt16; \
  		txt16.Copy(txt8); \
  		User::Panic( txt16 , __LINE__ ); }
#else
#define __PANIC(p)	{ \
					RDebug::Print(_L("WARNING!!! pointer to '%s' was NULL"), #p ); \
					RDebug::Print(_L("WARNING!!! file %s line %d"), __FILE__, __LINE__ ); \
					User::Panic( _L(__FILE__) , __LINE__ ); \
					}

#endif	// _UNICODE

#endif	// __PANIC

#if defined(__WINS__) || defined(_DEBUG)
#define __CHECK_NULL(p) if(p == NULL) __PANIC(p)
#else
#define __CHECK_NULL(p)
#endif


//
// size of arrays
//
#ifndef ELEM_SIZE
#define ELEM_SIZE(t) (sizeof(t)/sizeof(t[0]))
#endif


//
//	Check var against mask, return bool type
//
#define BOOL_BIT(var,msk) ( (var & msk)==(msk) )
//#define BOOL_BIT(var,msk)	(var & msk)
//#pragma warning( disable : 4800 )  // Disable warning messages

#if _DEBUG
#define DBG(a)	RDebug::Print(_L(a));
#define DBGF	RDebug::Print
#else
#define DBG(a)
#define DBGF	(void)
#endif

//#define HEAVY_DEBUGGING

#if defined(HEAVY_DEBUGGING) && defined(_DEBUG)
#define DBGARM(a)	RDebug::Print(_L(a));
#else
#define DBGARM(a)
#endif

//
// Profiling - log ticks
//
#if _DEBUG
#define DUMP_TICK(a) RDebug::Print(_L("tick %d: %d"), a, User::TickCount() );
#else
#define DUMP_TICK(a)
#endif


#endif // ETOOLS_H
