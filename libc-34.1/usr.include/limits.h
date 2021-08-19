/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _LIMITS_H
#define _LIMITS_H

#define CHAR_BIT (8)
#define SCHAR_MIN (-128)
#define SCHAR_MAX (127)
#define UCHAR_MAX (255U)

#ifdef __CHAR_UNSIGNED__
#define CHAR_MIN (0)
#define CHAR_MAX (UCHAR_MAX)
#else
#define CHAR_MIN (SCHAR_MIN)
#define CHAR_MAX (SCHAR_MAX)
#endif

#define MB_LEN_MAX (1)

#define SHRT_MIN (-32768)
#define SHRT_MAX (32767)
#define USHRT_MAX (65535U)

#define INT_MIN (-2147483648)
#define INT_MAX (2147483647)
#define UINT_MAX (4294967295U)

#define LONG_MIN (-2147483648)
#define LONG_MAX (2147483647)
#define ULONG_MAX (4294967295U)

#endif /* _LIMITS_H */
