#include "arm_muls.h"

//#if !(defined(__SYMBIAN32__) && defined(__GCCE__)) || defined(__WINSCW__) || defined(__WINS__)
#if 1

long mul(long x, long y)
{
  long long a = x;
  long long b = y;
  long long c = a*b;

  return (long)(c>>32);
}

long muls(long x, long y)
{
  long long a = x;
  long long b = y;
  long long c = a*b;

  return (long)((c<<1)>>32);
}

long mulr(long x, long y)
{
  long long a = x;
  long long b = y;
  long long c = a*b;

  return (long)((c+0x80000000)>>32);
}

long mulsr(long x, long y)
{
  long long a = x;
  long long b = y;
  long long c = a*b;

  return (long)(((c<<1)+0x80000000)>>32);
}

#else
/* ARMCC ones come from header file */
#endif
