#define	ESIZE	512
#define	NBRA	9

struct regex {
	char	expbuf[ESIZE];
	char	*braslist[NBRA];
	char	*braelist[NBRA];
	char	circf;
	char	*start, *end; /* pointers to occurrence in 's' */
};

char *re_comp (char *string);
int re_exec (char *string);
int recmp (char *pattern, char *target);
struct regex *re_compile (char *string, int fold);
int re_match (char *string, struct regex *r);
