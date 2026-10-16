/* Copyright (c) 1992 NeXT Computer, Inc. */

#ifndef _ANSI_I386_LIMITS_H
#define _ANSI_I386_LIMITS_H

#define CHAR_BIT 8
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255U

#ifdef __CHAR_UNSIGNED__
#define CHAR_MIN 0
#define CHAR_MAX UCHAR_MAX
#else
#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX
#endif

#define MB_LEN_MAX 1

#define SHRT_MIN (-32768)
#define SHRT_MAX 32767
#define USHRT_MAX 65535U

#define INT_MIN (-INT_MAX-1)
#define INT_MAX 2147483647
#define UINT_MAX 4294967295U

#define LONG_MIN INT_MIN
#define LONG_MAX INT_MAX
#define ULONG_MAX UINT_MAX

/* Minimum and maximum values a `signed long long int' can hold.  */
#define LONG_LONG_MIN (-LONG_LONG_MAX-1)
#define LONG_LONG_MAX 9223372036854775807LL

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0).  */
#define ULONG_LONG_MAX 18446744073709551615ULL

#endif /* _ANSI_I386_LIMITS_H */
