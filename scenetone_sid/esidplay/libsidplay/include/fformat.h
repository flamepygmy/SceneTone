//
// /home/ms/sidplay/libsidplay/include/RCS/fformat.h,v
//

#ifndef __FFORMAT_H
#define __FFORMAT_H


#if defined(__BORLANDC__)
  #include <strstrea.h>
#else
//  #include <strstream.h> //ALFRED - TODO
#endif
#include "mytypes.h"

#if defined(linux) || defined(unix)
  #define stricmp strcasecmp
#endif

extern int myStrNcaseCmp( char* pDestStr, const char* pSourceStr );
extern int myStrCaseCmp( char* string1, char* string2 );
extern char* myStrDup(const char *source);
extern char* fileNameWithoutPath(char* s);
extern char* fileExtOfFilename(char* s);
//extern udword readHex( istrstream& parseStream ); //ALFRED - TODO
//extern udword readDec( istrstream& parseStream ); //ALFRED - TODO
extern char* returnNextLine( char* pBuffer );
//extern void skipToEqu( istrstream& parseStream );//ALFRED - TODO
extern void copyStringValueToEOL( char* pSourceStr, char* pDestStr, int destMaxLen );


#endif
