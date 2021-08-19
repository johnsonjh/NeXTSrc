#ifdef	NeXT_MOD
#define	MAX		1024
#define	Int		register int
#define	Char 	register char
#define	Case		break; case
#define	Default	break; default
#define	null(s)	(!s || !*s)
static char *__arg, *__argp, *av0;
#define	argument	(__arg = (*__argp? __argp : av[++i==ac? --i : i]),__argp+=strlen(__argp), __arg)
#define	for_each_argument av0 = av[0]; for (i=1;i<ac && *av[i]=='-';i++) \
			for (__argp = &av[i][1]; *__argp;) \
			    switch(*__argp++)
#define	sp(c)	(c && (c==' ' || c=='\t'))
#define	skipspace(s) while sp(*s) ++s
#ifndef isdigit
#define isdigit(c)	(c>='0'&&c<='9')
#endif
char *index(), *strcpy(), *malloc();
#define	strsave(s) strcpy(malloc(strlen(s)+1),s)
#else	NeXT_MOD
#define d define
#d MAX		1024
#d Int		register int
#d Char 	register char
#d Case		break; case
#d Default	break; default
#d null(s)	(!s || !*s)
static char *__arg, *__argp, *av0;
#d argument	(__arg = (*__argp? __argp : av[++i==ac? --i : i]),__argp+=strlen(__argp), __arg)
#d for_each_argument av0 = av[0]; for (i=1;i<ac && *av[i]=='-';i++) \
			for (__argp = &av[i][1]; *__argp;) \
			    switch(*__argp++)
#d sp(c)	(c && (c==' ' || c=='\t'))
#d skipspace(s) while sp(*s) ++s
#ifndef isdigit
#define isdigit(c)	(c>='0'&&c<='9')
#endif
char *index(), *strcpy(), *malloc();
#d strsave(s) strcpy(malloc(strlen(s)+1),s)
#endif	NeXT_MOD
