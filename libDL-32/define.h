#ifndef DEFS
#define DEFS

#import <libc.h>
#import <string.h>
#import <stdlib.h>

#define NONE	(-1)
#define ERROR	(-2)

#define MAXLINE	1024
#define TEXTSIZE 2048

#define KEYFLAG '_'
#define NUMFLAG '#'

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef YES
#define YES	1
#endif

#ifndef NO
#define NO	0
#endif

#ifndef MIN
#define MIN(a,b) (a < b? a : b)
#endif

#ifndef MAX
#define MAX(a,b) (a > b? a : b)
#endif

#define	NULLCHAR	'\0'
#define STREQUAL(str1, str2)	(strcmp(str1, str2) == 0)
#endif
