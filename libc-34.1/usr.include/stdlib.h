/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __STRICT_BSD__
#error This file should not be in a strictly BSD program
#endif

#include <stddef.h> /* get size_t, NULL, etc. */

/*
 * match definition in /usr/include/sys/errno.h 
 */
#define	ERANGE		34		/* Result too large */
/*
 * match definition in /usr/include/math.h 
 */
#define HUGE_VAL (1e999)
#define EXIT_FAILURE (1)
#define EXIT_SUCCESS (0)
#define RAND_MAX (2147483647)
#define MB_CUR_MAX (1)

typedef struct {int quot, rem;} div_t;
typedef struct {long int quot, rem;} ldiv_t;

extern double atof(const char *nptr);
#define atof(nptr) strtod(nptr, (char **)NULL)
extern int atoi(const char *nptr);
extern long int atol(const char *nptr);
extern double strtod(const char *nptr, char **endptr);
extern long int strtol(const char *nptr, char **endptr, int base);
extern unsigned long int strtoul(const char *nptr, char **endptr, int base);
extern int rand(void);
extern void srand(unsigned int seed);
extern void *calloc(size_t nmemb, size_t size);
extern void free(void *ptr);
extern void *malloc(size_t size);
extern void *realloc(void *ptr, size_t size);
#ifndef __STRICT_ANSI__
extern void *valloc(size_t size);
extern void *alloca(size_t size);

#undef 	alloca
#define	alloca(x)	__builtin_alloca(x)

extern void cfree(void *ptr);
extern void vfree(void *ptr);
extern size_t malloc_size(void *ptr);
extern size_t malloc_good_size(size_t byteSize);
extern int malloc_debug(int level);
#ifdef	__cplusplus
typedef void (*_cplus_fcn_int)(int);
extern void (*malloc_error(_cplus_fcn_int))(int);
#else
extern void (*malloc_error(void (*fcn)(int)))(int);
#endif
extern size_t mstats(void);
#endif
extern void abort(void);
#ifdef	__cplusplus
typedef void (*_cplus_fcn_void)(void);
extern int atexit(_cplus_fcn_void);
#else
extern int atexit(void (*fcn)(void));
#endif
#if	defined(__GNUC__)  && !defined(__STRICT_ANSI__)
extern volatile void exit(int status);
#else
extern void exit(int status);
#endif	defined(__GNUC__)  && !defined(__STRICT_ANSI__)
extern char *getenv(const char *name);
extern int system(const char *string);
extern void *bsearch(const void *key, const void *base,
	size_t nmemb, size_t size,
	int (*compar)(const void *, const void *));
extern void *qsort(void *base, size_t nmemb, size_t size,
	int (*compar)(const void *, const void *));
extern int abs(int j);
extern div_t div(int numer, int denom);
extern long int labs(long int j);
extern ldiv_t ldiv(long int numer, long int denom);
extern int mblen(const char *s, size_t n);
extern int mbtowc(wchar_t *pwc, const char *s, size_t n);
extern int wctomb(char *s, wchar_t wchar);
extern size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n);
extern size_t wcstombs(char *s, const wchar_t *pwcs, size_t n);

#endif /* _STDLIB_H */

