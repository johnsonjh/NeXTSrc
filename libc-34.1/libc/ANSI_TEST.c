/*
 * This file contains code that serves two purposes:
 * 1) It contains a call to all ANSI C routines to verify
 *    that the required arguments are properly defined in
 *    the header files
 * 2) The object file can be used to find all symbols that
 *    must be hidden in order to conform to the ANSI C
 *    specification.
 *
 * This code is not meant to be run! It checks none of the
 * semantics of the routines, merely the header file declarations.
 */

#include <errno.h>
#include <stddef.h>
static void stddef_check(void) {
	ptrdiff_t pd;
	size_t st;
	wchar_t wc;
	void *nl;
	char *s;
	struct foo {
	  int x, y;
	};

	pd = s - s;
	st = sizeof(size_t);
	wc = 0;
	nl = NULL;
	st = offsetof(struct foo, y);
	errno = 0;
}

#include <assert.h>

static void assert_check(void) {
	int expression;

	assert(expression);
}

#include <ctype.h>

static void ctype_check(void) {
	int c;

	/* test as macros */
	c = isalnum(c);
	c = isalpha(c);
	c = iscntrl(c);
	c = isdigit(c);
	c = isgraph(c);
	c = islower(c);
	c = isprint(c);
	c = ispunct(c);
	c = isspace(c);
	c = isupper(c);
	c = isxdigit(c);
	c = tolower(c);
	c = toupper(c);

	/* test as functions */
	c = (isalnum)(c);
	c = (isalpha)(c);
	c = (iscntrl)(c);
	c = (isdigit)(c);
	c = (isgraph)(c);
	c = (islower)(c);
	c = (isprint)(c);
	c = (ispunct)(c);
	c = (isspace)(c);
	c = (isupper)(c);
	c = (isxdigit)(c);
	c = (tolower)(c);
	c = (toupper)(c);
}

#include <locale.h>

static void locale_check(void) {
	int category;
	const char *locale = "C";
	char *r;
	struct lconv *rl;

	/* test as macros */
	r = setlocale(LC_ALL, locale);
	r = setlocale(LC_COLLATE, locale);
	r = setlocale(LC_CTYPE, locale);
	r = setlocale(LC_MONETARY, locale);
	r = setlocale(LC_NUMERIC, locale);
	r = setlocale(LC_TIME, locale);
	rl = localeconv();
	
	/* test as functions */
	r = (setlocale)(LC_ALL, locale);
	r = (setlocale)(LC_COLLATE, locale);
	r = (setlocale)(LC_CTYPE, locale);
	r = (setlocale)(LC_MONETARY, locale);
	r = (setlocale)(LC_NUMERIC, locale);
	r = (setlocale)(LC_TIME, locale);
	rl = (localeconv)();
}

#include <math.h>

static void math_check(void) {
        int e;
	double d;

	e = EDOM;
	e = ERANGE;
	d = HUGE_VAL;
	errno = 0;
	/* test as macros */
	d = acos(d);
	d = asin(d);
	d = atan(d);
	d = atan2(d, d);
	d = cos(d);
	d = sin(d);
	d = tan(d);
	d = cosh(d);
	d = sinh(d);
	d = tanh(d);
	d = exp(d);
	d = frexp(d, &e);
	d = ldexp(d, e);
	d = log(d);
	d = log10(d);
	d = modf(d, &d);
	d = pow(d, d);
	d = sqrt(d);
	d = ceil(d);
	d = fabs(d);
	d = floor(d);
	d = fmod(d, d);

	/* test as functions */
	d = (acos)(d);
	d = (asin)(d);
	d = (atan)(d);
	d = (atan2)(d, d);
	d = (cos)(d);
	d = (sin)(d);
	d = (tan)(d);
	d = (cosh)(d);
	d = (sinh)(d);
	d = (tanh)(d);
	d = (exp)(d);
	d = (frexp)(d, &e);
	d = (ldexp)(d, e);
	d = (log)(d);
	d = (log10)(d);
	d = (modf)(d, &d);
	d = (pow)(d, d);
	d = (sqrt)(d);
	d = (ceil)(d);
	d = (fabs)(d);
	d = (floor)(d);
	d = (fmod)(d, d);
	e = errno;
}

#include <setjmp.h>

static void setjmp_check(void) {
	jmp_buf env;
	int r;
	
	r = setjmp(env);
	if (r == 0) {
	    longjmp(env, r);
	}

	r = (setjmp)(env);
	if (r == 0) {
	    (longjmp)(env, r);
	}
}

#include <signal.h>

static void signal_check(void) {
	sig_atomic_t x;
	int sig;
	typedef void (*pfv)(int);
	pfv f;
	int r;

	sig = SIGABRT;
	sig = SIGFPE;
	sig = SIGILL;
	sig = SIGINT;
	sig = SIGSEGV;
	sig = SIGTERM;
	/* test as macros */
	f = signal(sig, f);
	f = signal(sig, SIG_DFL);
	f = signal(sig, SIG_ERR);
	f = signal(sig, SIG_IGN);
	r = raise(sig);

	/* test as functions */
	f = (signal)(sig, f);
	f = (signal)(sig, SIG_DFL);
	f = (signal)(sig, SIG_ERR);
	f = (signal)(sig, SIG_IGN);
	r = (raise)(sig);
}

#include <stdarg.h>

static void stdarg_check(char *fmt, ...) {
	va_list ap;
	int x;

	/* test as macros */
	va_start(ap, fmt);
	x = va_arg(ap, int);
	va_end(ap);

	/* test as functions */
	va_start(ap, fmt);
	x = va_arg(ap, int);
	(va_end)(ap);
}

#include <stdio.h>

static void stdio_check(void) {
	FILE *f;
	fpos_t fp;
	const fpos_t cfp;
	int i;
	const char *cs;
	char *s;
	size_t size;
	va_list arg;
	void *vp;
	long int li;

	i = _IOFBF;
	i = _IOLBF;
	i = _IONBF;
	i = BUFSIZ;
	i = EOF;
	i = FOPEN_MAX;
	i = FILENAME_MAX;
	i = L_tmpnam;
	i = SEEK_CUR;
	i = SEEK_END;
	i = SEEK_SET;
	i = TMP_MAX;
	f = stderr;
	f = stdin;
	f = stdout;

	/* test as functions */
	i = remove(cs);
	i = rename(cs, cs);
	f = tmpfile();
	s = tmpnam(s);
	i = fclose(f);
	i = fflush(f);
	f = fopen(cs, cs);
	f = freopen(cs, cs, f);
	setbuf(f, s);
	i = setvbuf(f, s, i, size);
	i = fprintf(f, cs);
	i = fscanf(f, cs);
	i = printf(cs);
	i = scanf(cs);
	i = sprintf(s, cs);
	i = sscanf(cs, cs);
	i = vfprintf(f, cs, arg);
	i = vprintf(cs, arg);
	i = vsprintf(s, cs, arg);
	i = fgetc(f);
	s = fgets(s, i, f);
	i = fputc(i, f);
	i = fputs(cs, f);
	i = getc(f);
	i = getchar();
	s = gets(s);
	i = putc(i, f);
	i = putchar(i);
	i = puts(cs);
	i = ungetc(i, f);
	size = fread(vp, size, size, f);
	size = fwrite(vp, size, size, f);
	i = fgetpos(f, &fp);
	i = fseek(f, li, i);
	i = fsetpos(f, &cfp);
	li = ftell(f);
	rewind(f);
	clearerr(f);
	i = feof(f);
	i = ferror(f);
	perror(cs);

	/* test as functions */
	i = (remove)(cs);
	i = (rename)(cs, cs);
	f = (tmpfile)();
	s = (tmpnam)(s);
	i = (fclose)(f);
	i = (fflush)(f);
	f = (fopen)(cs, cs);
	f = (freopen)(cs, cs, f);
	(setbuf)(f, s);
	i = (setvbuf)(f, s, i, size);
	i = (fprintf)(f, cs);
	i = (fscanf)(f, cs);
	i = (printf)(cs);
	i = (scanf)(cs);
	i = (sprintf)(s, cs);
	i = (sscanf)(cs, cs);
	i = (vfprintf)(f, cs, arg);
	i = (vprintf)(cs, arg);
	i = (vsprintf)(s, cs, arg);
	i = (fgetc)(f);
	s = (fgets)(s, i, f);
	i = (fputc)(i, f);
	i = (fputs)(cs, f);
	i = (getc)(f);
	i = (getchar)();
	s = (gets)(s);
	i = (putc)(i, f);
	i = (putchar)(i);
	i = (puts)(cs);
	i = (ungetc)(i, f);
	size = (fread)(vp, size, size, f);
	size = (fwrite)(vp, size, size, f);
	i = (fgetpos)(f, &fp);
	i = (fseek)(f, li, i);
	i = (fsetpos)(f, &cfp);
	li = (ftell)(f);
	(rewind)(f);
	(clearerr)(f);
	i = (feof)(f);
	i = (ferror)(f);
	(perror)(cs);
}

#include <stdlib.h>

static void stdlib_check(void) {
	div_t dt;
	ldiv_t ldt;
	int i;
	unsigned int ui;
	long int li;
	unsigned long int uli;
	double d;
	const char *cs;
	char *s;
	char **endptr;
	size_t size;
	void *vp;
	const void *cvp;
	void (*pfv)(void);
	int (*compar)(const void *, const void *);
	wchar_t *ws;
	const wchar_t *cws;
	wchar_t wc;

	i = ERANGE;
	d = HUGE_VAL;
	i = EXIT_FAILURE;
	i = EXIT_SUCCESS;
	i = RAND_MAX;
	i = MB_CUR_MAX;
	
	/* test as functions */
	d = (atof)(cs);
	i = (atoi)(cs);
	li = (atol)(cs);
	d = (strtod)(cs, endptr);
	li = (strtol)(cs, endptr, i);
	uli = (strtoul)(cs, endptr, i);
	i = (rand)();
	(srand)(ui);
	vp = (calloc)(size, size);
	(free)(vp);
	vp = (malloc)(size);
	vp = (realloc)(vp, size);
	(abort)();
	i = (atexit)(pfv);
	(exit)(i);
	s = (getenv)(cs);
	i = (system)(cs);
	vp = (bsearch)(cvp, cvp, size, size, compar);
	(qsort)(vp, size, size, compar);
	i = (abs)(i);
	dt = (div)(i, i);
	li = (labs)(li);
	ldt = (ldiv)(li, li);
	i = (mblen)(cs, size);
	i = (mbtowc)(ws, cs, size);
	i = (wctomb)(s, wc);
	size = (mbstowcs)(ws, cs, size);
	size = (wcstombs)(s, cws, size);
}

#include <string.h>

static void string_check(void) {
	char *s;
	const char *cs;
	size_t size;
	int i;
	void *vp;
	const void *cvp;

	vp = (memcpy)(vp, cvp, size);
	vp = (memmove)(vp, cvp, size);
	s = (strcpy)(s, cs);
	s = (strncpy)(s, cs, size);
	s = (strcat)(s, cs);
	s = (strncat)(s, cs, size);
	i = (memcmp)(cvp, cvp, size);
	i = (strcmp)(cs, cs);
	i = (strncmp)(cs, cs, size);
	size = (strcoll)(s, size, cs);
	vp = (memchr)(cvp, i, size);
	s = (strchr)(cs, i);
	size = (strcspn)(cs, cs);
	s = (strpbrk)(cs, cs);
	s = (strrchr)(cs, i);
	size = (strspn)(cs, cs);
	s = (strstr)(cs, cs);
	s = (strtok)(s, cs);
	vp = (memset)(vp, i, size);
	s = (strerror)(i);
	size = (strlen)(cs);
}

#include <time.h>

static void time_check(void) {
	clock_t ct;
	time_t tt;
	const time_t ctt;
	struct tm tms;
	const struct tm ctms;
	double d;
	char *s;
	const char *cs;
	size_t size;
	struct tm *ptms;

	ct = CLK_TCK;
	ct = (clock)();
	d = (difftime)(tt, tt);
	tt = (mktime)(&tms);
	tt = (time)(&tt);
	s = (asctime)(&ctms);
	s = (ctime)(&ctt);
	ptms = (gmtime)(&ctt);
	ptms = (localtime)(&ctt);
	size = (strftime)(s, size, cs, &ctms);
}

static void kludge(void) {
#undef errno
extern volatile int errno;
	errno = 0;
}

main() {
	exit(0);
}
