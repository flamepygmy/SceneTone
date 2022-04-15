//
// /home/ms/sidplay/include/RCS/smart.h,v
//

#ifndef SMART__H //ALFRED
#define SMART__H //ALFRED

//#include <iostream.h> //ALFRED
//#include <iomanip.h> //ALFRED
#include <ctype.h>
#include "mytypes.h"


template <class T>
class smartPtrBase
{
 public:  // -----------------------------------------------------------------
	
	// --- constructors ---
	
	smartPtrBase(T* pBuffer, udword bufferLen)  
	{
		doFree = false;
		if ( bufferLen >= 1 )
		{
			pBufCurrent = ( pBufBegin = pBuffer );
			pBufEnd = pBufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
			pBufCurrent = ( pBufBegin = ( pBufEnd = 0 ));
			bufLen = 0;
			status = false;
		}
	}
	
	smartPtrBase(udword bufferLen)  
	{
		doFree = false;
		if ( bufferLen >= 1 )
		{
			pBufCurrent = ( pBufBegin = new T[bufferLen] );
			if ( pBufBegin == 0 )
			{
				status = false;
			}
			doFree = true;
			pBufEnd = pBufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
			pBufCurrent = ( pBufBegin = ( pBufEnd = 0 ));
			bufLen = 0;
			status = false;
		}
	}
	
	// --- destructor ---
	
	virtual ~smartPtrBase()
	{
		if ( doFree && (pBufBegin != 0) )
		{
			delete[] pBufBegin;
		}
	}
	
	// --- public member functions ---
	
    T* tellBegin()  { return pBufBegin;	} //ALFRED - removed 'virtual' keyword
	udword tellLength()  { return bufLen; } //ALFRED - removed 'virtual' keyword
	udword tellPos()  { return (udword)(pBufCurrent-pBufBegin); } //ALFRED - removed 'virtual' keyword
  
	bool reset(T element) //ALFRED - removed 'virtual' keyword
	{
		if ( bufLen >= 1 )
		{
			pBufCurrent = pBufBegin;
			return (status = true);
		}
		else
		{
			return (status = false);
		}
	}

	bool good() //ALFRED - removed 'virtual' keyword
	{
		if ( pBufCurrent < pBufEnd )
			return true;
		else
			return false;
	}
	
	bool fail() //ALFRED - removed 'virtual' keyword 
	{
		if ( pBufCurrent == pBufEnd )
			return true;
		else
			return false;
	}
	
	void operator ++() //ALFRED - removed 'virtual' keyword
	{
		if ( status && !fail() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	void operator ++(int) //ALFRED - removed 'virtual' keyword
	{
		if ( status && !fail() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	void operator --() //ALFRED - removed 'virtual' keyword
	{
		if ( status && (pBufCurrent > pBufBegin) )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	void operator --(int) //ALFRED - removed 'virtual' keyword
	{
		if ( status && (pBufCurrent > pBufBegin) )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	void operator +=(udword offset) //ALFRED - removed 'virtual' keyword
	{
		if ( status && ((pBufCurrent + offset) < pBufEnd) )
		{
			pBufCurrent += offset;
		}
		else
		{
			status = false;
		}
	}
	
	void operator -=(udword offset) //ALFRED - removed 'virtual' keyword
	{
		if ( status && ((pBufCurrent - offset) >= pBufBegin) )
		{
			pBufCurrent -= offset;
		}
		else
		{
			status = false;
		}
	}
	
	T operator*()
	{
		if ( status && good() )
		{
			return *pBufCurrent;
		}
		else
		{
			status = false;
			return dummy;
		}
	}

	T& operator [](udword index)
	{
		if ( status && ((pBufCurrent + index) < pBufEnd) )
		{
			return pBufCurrent[index];
		}
		else
		{
			status = false;
			return dummy;
		}
	}

#if !defined(__BORLANDC__)
//	operator int()  { return status; } //ALFRED - removed 'virtual' keyword
#endif
	operator bool()  { return status; } //ALFRED - removed 'virtual' keyword
	
 protected:  // --------------------------------------------------------------
	
	T* pBufBegin;
	T* pBufEnd;
	T* pBufCurrent;
	udword bufLen;
	bool status;
	bool doFree;
	T dummy;
};


template <class TP>
class smartPtr : public smartPtrBase<TP>
{
 public:  // -----------------------------------------------------------------
	
	// --- constructors ---
	
	smartPtr(TP* pBuffer, udword bufferLen) : smartPtrBase<TP>(pBuffer, bufferLen)
	{
	}
	
	smartPtr(udword bufferLen) : smartPtrBase<TP>(bufferLen)
	{
	}
	
	// --- public member functions ---
	
};


#endif // SMART__H //ALFRED
