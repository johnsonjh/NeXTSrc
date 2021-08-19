# 1 "strtod.c"
# 1 "/BSD/LIBS/libs-8/usr/include/ctype.h"
 

 




extern int isalnum(int c);
extern int isalpha(int c);
extern int iscntrl(int c);
extern int isdigit(int c);
extern int isgraph(int c);
extern int islower(int c);
extern int isprint(int c);
extern int ispunct(int c);
extern int isspace(int c);
extern int isupper(int c);
extern int isxdigit(int c);
extern int tolower(int c);
extern int toupper(int c);










extern	char	_ctype_[];





























# 1 "strtod.c"

# 1 "/BSD/LIBS/libs-8/usr/include/string.h"
 



# 1 "/BSD/LIBS/libs-8/usr/include/stddef.h"
 










typedef long ptrdiff_t;
typedef unsigned long size_t;



extern volatile int *_errno();
extern volatile int errno;





extern int sys_nerr;
extern char *sys_errlist[];



# 5 "/BSD/LIBS/libs-8/usr/include/string.h"


 

extern void *memcpy(void *s1, const void *s2, size_t n);
extern void *memmove(void *s1, const void *s2, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern size_t strcoll(char *to, size_t maxsize, const char *from);
extern void *memchr(const void *s, int c, size_t n);
extern char *strstr(const char *s1, const char *s2);
extern void *memset(void *s, int c, size_t n);
extern char *strerror(int errnum);


 
extern char *strcpy(char *s1, const char *s2);
extern char *strcat(char *s1, const char *s2);
extern int strcmp(const char *s1, const char *s2);
extern char *strchr(const char *s, int c);
extern char *strpbrk(const char *s1, const char *s2);
extern char *strrchr(const char *s, int c);
extern char *strtok(char *s1, const char *s2);

 








extern char *strncpy(char *s1, const char *s2, size_t n);
extern char *strncat(char *s1, const char *s2, size_t n);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern size_t strcspn(const char *s1, const char *s2);
extern size_t strspn(const char *s1, const char *s2);
extern size_t strlen(const char *s);



 
extern char *index(const char *s, int c);
extern char *rindex(const char *s, int c);






 
extern char *strcpyn();
extern char *strcatn();
extern int strcmpn();



# 2 "strtod.c"

# 1 "/BSD/LIBS/libs-8/usr/include/stddef.h"
 

# 28 "/BSD/LIBS/libs-8/usr/include/stddef.h"

# 3 "strtod.c"

# 1 "/BSD/LIBS/libs-8/usr/include/math.h"
 

 

 


# 1 "/BSD/LIBS/libs-8/usr/include/stdlib.h"
 



# 1 "/BSD/LIBS/libs-8/usr/include/stddef.h"
 

# 28 "/BSD/LIBS/libs-8/usr/include/stddef.h"

# 5 "/BSD/LIBS/libs-8/usr/include/stdlib.h"


 

 


typedef struct {int quot, rem;} div_t;
typedef struct {long int quot, rem;} ldiv_t;

extern double atof(const char *nptr);

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
extern void abort(void);
extern int atexit(void (*func)(void));
extern void exit(int status);
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


# 8 "/BSD/LIBS/libs-8/usr/include/math.h"



 




 

extern double asinh(double x), acosh(double x), erf(double x), erfc(double x);
extern double lgamma(double x), hypot(double x, double y);
extern double cabs(struct {double x,y;} z), copysign(double x, double y);
extern int finite(double x);
extern double j0(double x), j1(double x), jn(int n, double x);
extern double y0(double x), y1(double x), yn(int n, double x);
extern double cbrt(double x);

extern double pow(double x, double y), floor(double x), ceil(double x);
extern double atan2(double x, double y);
extern double drem(double x, double y);
extern double scalb(double x, int n);

 

extern double strtod(char *x, (char **)((void *)0) ) ;

extern double modf(double value, double *iptr);
extern double ldexp(double value, int exp);
extern double frexp(double value, int *eptr);

 

extern double atanh(double x), expm1(double x), logb(double x);
extern double log1p(double x), rint(double x);

extern double acos(double x), asin(double x), atan(double x);
extern double cos(double x), cosh(double x), exp(double x);
extern double fabs(double x), log(double x), log10(double x);
extern double sin(double x), sinh(double x), sqrt(double x);
extern double tan(double x), tanh(double x);



static inline double _builtin_acos(double x)  { double v; asm("facosx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_asin(double x)  { double v; asm("fasinx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _building_atan(double x)  { double v; asm("fatanx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _building_atanh(double x)  { double v; asm("fatanhx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_cos(double x)  { double v; asm("fcosx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_cosh(double x)  { double v; asm("fcoshx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_exp(double x)  { double v; asm("fetoxx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_expm1(double x)  { double v; asm("fetoxm1x %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_fabs(double x)  { double v; asm("fabsx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_log(double x)  { double v; asm("flognx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_logb(double x)  { double v; asm("fgetexpx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_log10(double x)  { double v; asm("flog10x %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_log1p(double x)  { double v; asm("flognp1x %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_rint(double x)  { double v; asm("fintx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_sin(double x)  { double v; asm("fsinx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_sinh(double x)  { double v; asm("fsinhx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_sqrt(double x)  { double v; asm("fsqrtx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_tan(double x)  { double v; asm("ftanx %1,%0": "=f" (v): "f" (x)); return v; }

static inline double _builtin_tanh(double x)  { double v; asm("ftanhx %1,%0": "=f" (v): "f" (x)); return v; }













































# 4 "strtod.c"


 


struct pd_fp {
	unsigned int
		pf_signman:1,
		pf_signexp:1,
		pf_infnan:2,
		pf_exp2:4,
		pf_exp1:4,
		pf_exp0:4,
		pf_exp3:4,
		pf_xxx0:4,
		pf_xxx1:4,
		pf_man16:4;
	unsigned int pf_man15_8;
	unsigned int pf_man7_0;
};




static unsigned int packmantissa();


double
strtod(const char *str, char **endptr) {
	register const char *p = str;
	struct pd_fp pd_fp;
	char mantissa[17 ];
	int left_of_decimal, exponent;
	int seen_non_zero;	 
	int seen_digit;
	unsigned int packmantissa();
	extern double _pdfptodbl();

	 
	pd_fp.pf_signman = 0;
	pd_fp.pf_signexp = 0;
	pd_fp.pf_infnan = 0;
	pd_fp.pf_xxx0 = 0;
	pd_fp.pf_xxx1 = 0;
	pd_fp.pf_exp3 = 0;
	exponent = 0;
	left_of_decimal = 0;
	seen_non_zero = 0;
	seen_digit = 0;
	memset(mantissa,0, sizeof(mantissa)); ;

	while(((int)((_ctype_+1)[*(p)]&010 )) ) (p)++ ;
	if (*p == '-') {
		p++;
		pd_fp.pf_signman = 1;
	} else if (*p == '+')
		p++;
	while(((int)((_ctype_+1)[*(p)]&010 )) ) (p)++ ;
	if (((int)((_ctype_+1)[*p]&(01 |02 ))) ) {
		union {
			double d;
			unsigned int x[2];
		} result;

		static const struct const_table_struct {
			const char *s;
			unsigned int x[2];
		} const_table[] = {
			"infinity", 0x7ff00000, 0x00000000,
			"inf",      0x7ff00000, 0x00000000,
			"nan",      0x7fffffff, 0xffffffff,
			"snan",     0x7ff7ffff, 0xffffffff,
			((void *)0) ,       0x00000000, 0x00000000,
		};
		
		const struct const_table_struct *c = const_table;
		
		do {
			const char *q = p;
			const char *r = c->s;
			
			do {
				if (*r == '\0') {
					p = q;
					goto const_string;
				}			
			} while (({int _c=(*q++); ((int)((_ctype_+1)[_c]&02 ))  ? ((int)((_c)-'A'+'a'))  : _c;})  == *r++);
		} while ((++c)->s != ((void *)0) );
const_string:	result.x[0] = c->x[0];
		result.x[1] = c->x[1];
		if (pd_fp.pf_signman)
			result.x[0] |= 0x80000000;
		if (endptr != ((void *)0) ) {
			*endptr = (c->s == ((void *)0) ) ? (char *)str : (char *)p;
		}
		return(result.d);
	}
	 



	while(((int)((_ctype_+1)[*p]&04 )) ) {
		seen_digit = 1;
		if (*p == '0' && !seen_non_zero) {
			p++;
			continue;
		}
		seen_non_zero = 1;
		mantissa[(17 -1)-left_of_decimal++]  = (*p++) - '0';
	}
	 
	if (*p == '.') {
		int mantindex = 17 -1-left_of_decimal;
		 
		p++;
		while(((int)((_ctype_+1)[*p]&04 )) ) {
			seen_digit = 1;
			if (!seen_non_zero && *p == '0') {
				p++;
				left_of_decimal--;
				continue;
			}
			seen_non_zero = 1;
			if (mantindex >= 0)
			    mantissa[mantindex--] = (*p++) - '0';
 			else
 			    p++;
		}
	}
	while(((int)((_ctype_+1)[*(p)]&010 )) ) (p)++ ;
	if (*p == 'e' || *p == 'E') {
		p++;
		while(((int)((_ctype_+1)[*(p)]&010 )) ) (p)++ ;
		 
		if (*p == '+')
			p++;
		else if (*p == '-') {
			p++;
			pd_fp.pf_signexp = 1;
		}
		while(((int)((_ctype_+1)[*(p)]&010 )) ) (p)++ ;
		 
		while(((int)((_ctype_+1)[*p]&04 )) ) {
			exponent *= 10;
			exponent += (*p++) - '0';
			 
			if (exponent > 100000)
				goto expover;
		}
	}
	 
	if (pd_fp.pf_signexp)
		exponent = -exponent;
	 
	exponent += left_of_decimal - 1;
	if (exponent < 0) {
		exponent = -exponent;
		pd_fp.pf_signexp = 1;
	} else
	         

	        pd_fp.pf_signexp = 0;
	 
	if (exponent > 999) {
		union {
			double d;
			unsigned int x[2];
		} inf;
expover:
		 
		(*_errno())  = 	34		;
		if (endptr != ((void *)0) ) {
			*endptr = seen_digit ? (char *)p : (char *)str;
		}
		if (pd_fp.pf_signexp)
			return(0.0);
		if (pd_fp.pf_signman)
			inf.x[0] = 0x7ff00000;
		else
			inf.x[0] = 0xfff00000;
		inf.x[1] = 0;
		return(inf.d);
	}
	pd_fp.pf_exp0 = exponent % 10;
	pd_fp.pf_exp1 = (exponent/10) % 10;
	pd_fp.pf_exp2 = (exponent/100) % 10;
	 
	pd_fp.pf_man16 = mantissa[16];
	pd_fp.pf_man15_8 = packmantissa(&mantissa[15]);
	pd_fp.pf_man7_0 = packmantissa(&mantissa[7]);
	if (endptr != ((void *)0) ) {
		*endptr = (char *)p;
	}
	return(_pdfptodbl(&pd_fp));
}

static unsigned int 
packmantissa(cp)
char *cp;
{
	register unsigned int result = 0;
	register int i;

	for(i=0;i<8;i++) {
		result <<= 4;
		result |= *cp--;
	}
	return(result);
}
