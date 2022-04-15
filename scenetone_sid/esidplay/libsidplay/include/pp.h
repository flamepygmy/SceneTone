//
// pp.h,v 1.1 1996/12/07 05:12:08 ms Exp
//

#ifndef __PP_H
  #define __PP_H


//#include <fstream.h> ALFRED
#include "mytypes.h"

//extern int depp( ifstream& inputFile, ubyte** destBufRef ); //ALFRED - TODO
extern udword ppUncompressedLen();
extern const char* ppErrorString;


#endif
