#if !defined(__ARM_MULS_H__)
#define __ARM_MULS_H__

#if 0 // defined(__SYMBIAN32__) && defined(__GCCE__) && !defined(__WINSCW__) && !defined(__WINS__) &&

static inline __attribute__ ((always_inline)) long mul(long x, long y)
{
    long out,temp;
    asm(
            "smull  %1,%0,%2,%3            \n\t"       \
            : "=&r" (out), "=&r" (temp)                \
            : "r"  (x), "r" (y)                        \
            : "0");

    return out;
}

static inline __attribute__ ((always_inline)) long mulr(long x, long y)
{
    long out,temp;
    asm(
        "smull   %1, %0, %2, %3            \n\t"       \
		"adds    %1, %1, #0x80000000       \n\t"       \
        "adc     %0, %0, #0                \n\t"       \
            : "=&r" (out), "=&r" (temp)                \
            : "r"  (x), "r" (y)                        \
            : "0");

    return out;
}

static inline __attribute__ ((always_inline)) long muls(long x, long y)
{
    long out,temp;
    asm(
        "smull   %1, %0, %2, %3           \n\t"        \
        "movs    %1, %1, lsl #1           \n\t"        \
        "adc     %0, %0, %0               \n\t"        \
            : "=&r" (out), "=&r" (temp)                \
            : "r"  (x), "r" (y)                        \
            : "0");

    return out;
}

static inline __attribute__ ((always_inline)) long mulsr(long x, long y)
{
    long out,temp;
    asm(
        "smull   %1, %0, %2, %3           \n\t"        \
		"movs    %1, %1, lsl #1           \n\t"        \
		"adc     %0, %0, %0               \n\t"        \
		"adds    %1, %1, #0x80000000      \n\t"        \
		"adc     %0, %0, #0               \n\t"        \
            : "=&r" (out), "=&r" (temp)                \
            : "r"  (x), "r" (y)                        \
            : "0");

    return out;
}

#else /* __GCCE__ */

extern long mul(long x, long y);
extern long mulr(long x, long y);
extern long mulsr(long x, long y);
extern long muls(long x, long y);

#endif

#endif /* __ARM_MULS_H__ */
