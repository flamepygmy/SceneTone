// EPOCGLUE.H
//
// (c) 1999-2000 Alfred E. Heggestad
//

#if !defined(EPOCGLUE_H__)
#define EPOCGLUE_H__

#include <string.h>
#include "elog.h"

#if defined (__SYMBIAN32__)

#ifndef __E32DEF_H__
  #ifdef NULL
  #undef NULL
  #endif
#endif

#include <e32base.h>
#include <e32std.h>

#else // not Epoc
#ifndef IMPORT_C
#define IMPORT_C
#endif

#ifndef EXPORT_C
#define EXPORT_C
#endif

#ifndef _L
#define _L
#endif

#endif

#include "etools.h" // anyway

#endif // EPOCGLUE_H__
