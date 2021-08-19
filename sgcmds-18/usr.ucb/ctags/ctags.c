#include <stdio.h>
#include <ctype.h>
/*#include <strings.h>*/
#include <sys/types.h>
#include <sys/stat.h>
#include "tags.h"
#undef sp

#define	NeXT_ASS	1	/* add support for NeXT assembler */

#define	TRUE	(1)
#define	FALSE	(0)

#define	iswhite(arg)	(_wht[arg])	/* T if char is white		*/
#define	begtoken(arg)	(_btk[arg])	/* T if char can start token	*/
#define	intoken(arg)	(_itk[arg])	/* T if char can be in token	*/
#define	endtoken(arg)	(_etk[arg])	/* T if char ends tokens	*/
#define	isgood(arg)	(_gd[arg])	/* T if char can be after ')'	*/

#define alloc(t)	(t *)calloc(1, sizeof (t))
#define substrcpy(to,from,len)	(strncpy(to, from, len)[len] = 0)

typedef enum {
	NOTYPE,
	FUNC,		/* subroutine or function definition */
	MACRO,		/* C-preprocessor macro definition */
	TYPEDEF,	/* C typedef definition */
	STRUCT,		/* C structure tag name */
	GLOBAL,		/* global variable */
	YACCTOKEN,	/* YACC non-terminal token name */
	
	METHOD,		/* objective-C method */
	CLASS,		/* objective-C class */
} TagType;

char *Typename[]= {
	"null",
	"function",
	"macro",
	"typedef",
	"struct",
	"global",
	"yacctoken",
	"method",
	"class",
};

typedef struct List {
	struct	List *next;
	char	*name;
} List;

/* Sorting structure. */
typedef struct nd_st {
	char	*entry;			/* function or type name	*/
	char	*file;			/* file name			*/
	TagType	type;			/* why this was tagged		*/
	char	f;			/* use pattern or line no	*/
	char	been_warned;		/* set if noticed dup		*/
	char	undecl;			/* node made by forward ref	*/
	int	lno;			/* for -x option		*/
	char	*pat;			/* search pattern		*/
	int	length;			/* of the object, in lines	*/
	struct	nd_st *left,*right;	/* left and right sons		*/
	List	*caller, *callee;	/* callees/callers of node	*/
	int	ncaller, ncallee;	/* length of these lists	*/
} NODE;

struct keyword {
	char	*name;
	int	hash;
} Ckword[] = {
	{ "enum" },
#define isenum(h,s)	(h == Ckword[0].hash ? !strcmp(s,Ckword[0].name) : 0)
	{ "extern" },
#define isextern(h,s)	(h == Ckword[1].hash ? !strcmp(s,Ckword[1].name) : 0)
	{ "struct" },
#define isstruct(h,s)	(h == Ckword[2].hash ? !strcmp(s,Ckword[2].name) : 0)
	{ "typedef" },
#define istypedef(h,s)	(h == Ckword[3].hash ? !strcmp(s,Ckword[3].name) : 0)
	{ "union" },
#define isunion(h,s)	(h == Ckword[4].hash ? !strcmp(s,Ckword[4].name) : 0)
	{ "static" }, 
#define isstatic(h,s)	(h == Ckword[5].hash ? !strcmp(s,Ckword[5].name) : 0)
	{ "auto" },	{ "break" },	{ "char" },	{ "case" },
	{ "continue" },	{ "double" },	{ "default" },	{ "define" },
	{ "else" },	{ "elseif" },	{ "float" },	{ "for" },
	{ "fortran" },	{ "goto" },	{ "if" },	{ "ifdef" },
	{ "ifndef" },	{ "int" },	{ "include" },	{ "long" },
	{ "return" },	{ "register" },	{ "switch" },	{ "sizeof" },
	{ "short" },	{ "unsigned" },	{ "undef" },	{ "void" },
	{ "while" },
	{ 0 }
};
/* prime chosen to be ~8*NCKEY */
#define	NCKEYHASH	293
struct	keyword *CkwordHash[NCKEYHASH];

char	number;			/* T if on line starting with #	*/
char	level;			/* brace { } nesting level	*/
char	gotmacro;		/* found a func already on line	*/
char	seenmactoken;		/* looked at first token of #cmd*/
				/* boolean "func" (see init)	*/
char	seenparen;		/* token was followed by parens	*/
char	_wht[0177];
char	_etk[0177];
char	_itk[0177];
char	_btk[0177];
char	_gd[0177];

char	searchar = '/';		/* use /.../ searches */
int	lineno;			/* line number of current line */
int	startline;		/* line number of start of typedef */
char	line[4*BUFSIZ];		/* current input line */
char	*curfile;		/* current input file name */
char	*outfile = "tags";	/* output file */

				/* command-line flags: */
int	append, update;
int	vgrind, cxref;
int	NoXrefs,NoGlobal,NoMacro,NoStruct,NoTypedef,NoWarn,NoDuplicates=1;
int	ObjectiveC = 0;
int	MaxRefs = 60;		/* max length of callee/caller lists */
char	lbuf[BUFSIZ];

FILE	*inf;			/* current input file */
FILE	*outf;			/* tags file */
long	lineftell;		/* ftell after getc(inf) == '\n' */

NODE	*head;			/* the head of the sorted binary tree */
NODE	*LastNode, *PrevLastNode = 0;	/* node currently being processed */
NODE 	*MainNode, *MmainNode;

char	*savestr(),*malloc(),*calloc(),*rindex(),*index(),*strcpy(),*strncpy();

typedef struct Inode {
	long n;
	struct Inode *next;
} Inode;
#define MAXI 273
Inode *itab[MAXI];

long
inode(fd){
	struct stat b;
	fstat(fd,&b);
	return (long)(b.st_ino);
}

seen(f) FILE *f; { /* true if inode(f) has been processed already */
	long n = inode(fileno(f));
	int h = n%MAXI;
	Inode *i = itab[h];
	for (;i && i->n != n; i = i->next) ;
	if (!i){
		i = (Inode *)malloc(sizeof(Inode));
		i->n = n;
		i->next = itab[h];
		itab[h] = i;
		return 0;
	}
	fclose(f);
	return 1;
}

#ifdef NeXT_ASS		/* For NeXT assembler macros */

#define	strpfx(a,b)	(strncmp(a,b,sizeof(b)-1)==0)

S_entries(fi) FILE *fi; {
	Char *cp;

	lineno = 0;
	while (fgets(lbuf, sizeof(lbuf), fi)) {
		lineno++;
		if (*lbuf == '#')
			check_cpp(lbuf);
		else {
			cp = lbuf;
			while (*cp && isspace(*cp))
				cp++;
			if (isupper(*cp)) {
				if (strpfx(cp, "PROCENTRY")
				    || strpfx(cp, "ENTRY(") /*)*/
				    || strpfx(cp, "ENTRY2(") /*)*/
				)
					S_getit(index(cp, '(') + 1, FUNC);
			}
	   	}
	}
}

check_cpp(cp)
	Char *cp;
{
	cp++;		/* skip leading # */
	while (isspace(*cp))
		cp++;
	if (strpfx(cp, "define")) {
		cp += 6;
		if (!isspace(*cp))
			return;
		while (isspace(*cp))
			cp++;
		if (!isalpha(*cp))
			return;
		S_getit(cp, MACRO);
	}
}

S_getit(np, kind)
	Char *np;
{
	Char *cp, c;
	char nambuf[BUFSIZ];

	for (cp = lbuf; *cp; cp++)
		continue;
	*--cp = 0;	/* zap newline */
	while (isspace(*np))
		np++;
	for (cp = np; isalnum(*cp) || *cp == '_'; cp++)
		continue;
	c = *cp;
	*cp = 0;
	strncpy(nambuf, np, sizeof(nambuf)-1);
	nambuf[sizeof(nambuf)-1] = 0;
	*cp = c;
	pfnote(nambuf, lineno, TRUE, kind);
}
#endif NeXT_ASS

find_entries(file)
	Char *file;
/*
 * This routine opens the specified file and calls the function
 * which finds the function and type definitions.
 */
{
	char *s;

	inf = fopen(file, "r");
	if (inf == NULL)  {
	    perror (file);
	    return (0);
	}
	if (seen(inf)) {
	    return e("%s: already indexed",file);
	}
	ObjectiveC = 0;
	lineno = 1;
	curfile = savestr(file);
	s = rindex(file, '.');
	if (s && s[2]=='\0' && (s[1] != 'c' && s[1] != 'h')) switch(s[1]){
	Case 'l': /* lisp or lex */
		if (index(";([", first_char())) { /* lisp */
			L_funcs(inf);
			goto done;
		}
		/* Lex, throw away all the code before the second "%%" */
		toss_yysec();
		getline();
		pfnote("yylex", lineno, TRUE, FUNC);
		toss_yysec();
	Case 'y': /* yacc */
		toss_yysec();
		Y_entries();
#ifdef NeXT_ASS
	Case 's': S_entries(inf); goto done;
#endif NeXT_ASS
	Case 'm': ObjectiveC = 1;
	Default: /* try fortran */
		if (PF_funcs(inf)) goto done;
		rewind(inf);	/* no fortran found; try C */
		lineno = 1;
	}
	C_entries();
done:
	fclose(inf);
}

NODE *
addtag(name, ln, f, type, undecl)
	Char *name, f, undecl;
	TagType	type;
{
	register NODE *n = alloc(NODE);
	if (!n){
		e("ctags: warning - too many entries to sort");
		put_entries(head);
		free_tree(head);
		head = n = alloc(NODE);
	}
	n->entry = savestr(name);
	n->file = curfile;
	n->f = 1;
	n->type = type;
	n->undecl = undecl;
	n->lno = ln;
	n->left = n->right = 0;
	n->caller = n->callee = 0;
	n->length = 1;
	/*if (f){
		if (!cxref) lbuf[40]='\0',strcat(lbuf, "$"),lbuf[40]='\0';
	} else
		printf("%s\n",lbuf), *lbuf = '\0';*/
	lbuf[50]='\0';
	n->pat = savestr(lbuf);
	if (!undecl) {
		if (number && level)
			PrevLastNode = LastNode;
		LastNode = n;
	}
	if (!head) head = n;
	else add_node(n, head);
	return n;
}

NODE *
findnode(s)
	Char *s;
{
	register NODE *n;
	Int dif;
	for (n = head; n; n = dif < 0 ? n->right : n->left) {
		dif = strcmp(n->entry, s);
		if (!dif) break;
	}
	return n;
}

NODE *
lookup(s) Char *s; {
	register NODE *n = findnode(s);
	return (n && !n->undecl) ? n : 0;
}

trynote(name, ln, type)
	Char *name;
	TagType type;
{
	if (!Ckeyword(name)){
		pfnote(name, ln, 0, type);
		if (LastNode) LastNode->length = lineno - ln + 1;
	}
}

pfnote(name, ln, f, type)
	Char *name, f;	/* f == TRUE when function or #define or method or class */
	TagType	type;
{
	Char *s;
	char nbuf[BUFSIZ];
	static int firstmain = 1;
	Int gotmain = 0, len;

	if (!cxref && !strcmp(name,"main")) {
		if (firstmain) {
			MainNode = addtag(name, ln, f, type, 0);
			len = strlen(lbuf)-1;
			if (lbuf[len] == '$' && len < 40)
				lbuf[len]='\0'; /* get rid of trailing $ */
			firstmain = 0, gotmain = 1;
		}
		s = (s=rindex(curfile, '/'))? s+1 : curfile;
		sprintf(nbuf, "M%s", s);
		s = rindex(nbuf, '.');
		if (s && s[2] == '\0')
			*s = '\0';
		name = nbuf;
	}
	if (gotmain)
		MmainNode = addtag(name, ln, f, type, 0);
	else
		addtag(name, ln, f, type, 0);
}

Ckeyword(s)
	Char *s;
{
	register struct keyword *p;
	Int h=hash(s);
	for (; p = CkwordHash[h%NCKEYHASH]; h <<= 1)
		if (h == p->hash && !strcmp(p->name, s))
			return TRUE;
	return FALSE;
}

C_entries(){ /* find functions/typedefs in C syntax and add them to the list */
	Char c, *token, *tp;
	char incomm = FALSE, inquote = FALSE, inchar = FALSE, midtoken = FALSE;
	char ininit = FALSE, inbracket = FALSE, intypedef= FALSE;
	char instruct = FALSE, inextern = FALSE, infunc = FALSE;
	char firstword = TRUE, Oincomm=FALSE, inmethod=FALSE, inclass=FALSE;
	char *sp, tok[BUFSIZ];

	number = gotmacro = FALSE;
	firstword = TRUE;
	lineftell = 0;
	level = 0;
	sp = tp = token = line;
	for (;;) {
		*sp = c = getc(inf);
		if (feof(inf)) break;
		if (c == '\n') lineno++;
		if (c == '\\') {
			*++sp = c = getc(inf);
			if (c == '\n')
				lineno++, c = ' ';
		} else if (incomm) {
			if (c == '*') {
				while ((*++sp = c = getc(inf)) == '*')
					;
				if (c == '\n') lineno++;
				if (c == '/') incomm = FALSE;
			}
		} else if (Oincomm) {
			if (c=='\n'){ Oincomm = FALSE; lineno++; }
		} else if (inquote) {
			if (c == '"') inquote = FALSE;
			continue;
		} else if (inchar) {
			if (c == '\'')
				inchar = FALSE;
			continue;
		} else switch (c) {
		case '"':
			inquote = TRUE;
			continue;
		case '\'':
			inchar = TRUE;
			continue;
		case '/': switch (*++sp = c = getc(inf)){
			  Case '*': incomm = TRUE;
	      		  Case '/': if (ObjectiveC) Oincomm = TRUE;
		  	  }
			  if (!incomm && !Oincomm) ungetc(*sp, inf);
			break;
		case '#':
			if (sp == line)
				number = TRUE, seenmactoken = FALSE;
			continue;
	    
		case '{': level++; continue;
		case '}':
			if (sp == line)
				level = 0;	/* reset */
			else if (level)
				level--;
			if (level == 0) {
				if (infunc)
					firstword = TRUE;
				if (infunc ||
				    (instruct && LastNode &&
				    LastNode->type == STRUCT &&
				    LastNode->lno == startline))
					 LastNode->length =
					     lineno - LastNode->lno + 1;
				infunc = FALSE;
			}
			continue;
		}
		if (ObjectiveC){
		  if (!level && !incomm && c==':'){
	  		while (iswhite(c=getc(inf)))
	    			if (c=='\n') ++lineno;
		    	while (intoken(c=getc(inf)))
				if (c=='\n') ++lineno;
	  	  } else
		  if(sp==line){
		    if (c=='+'||c=='-'||c=='='){
			if (c=='=') inclass=TRUE; else inmethod=TRUE;
	    		if (inmethod){
				while (iswhite(c=getc(inf)))
					if (c=='\n') ++lineno;
				ungetc(c,inf);
				if (c=='('){
					while (c != ')'){
		    				c=getc(inf);
			    			if (c=='\n') ++lineno;
			    		}
		    		}
		    	}
		    } else
	    	    if (c=='@' && sp==line){
			char buf[128], *p = buf;
			while(iswhite(c=getc(inf)))
				if (c=='\n') ++lineno;
			while ((c=getc(inf)) != EOF && !endtoken(c)){
				*p++ = c;
			}
			*p = 0;
			if (strncmp(buf,"implementation",14)==0)
				inclass = TRUE;
			else
			if (strncmp(buf,"interface",9)==0)
				inclass = TRUE;
			if (c == EOF) return;
	    		continue;
		    }
		  }
	  	}
		if ((!inbracket || ObjectiveC) && !NoXrefs &&
		    ((infunc && level && !number) || (number && gotmacro)) &&
		    !incomm && !Oincomm && !inquote) {
			if (midtoken) {
				if (endtoken(c)) {
					substrcpy(tok,token,tp-token+1);
					while (iswhite(c) && c != '\n')
						c = getc(inf);
					if (c != '\n')
						(void) ungetc(c, inf);
					calls(tok);
					midtoken = FALSE;
					token = sp;
				} else if (intoken(c))
					tp++;
			} else if (begtoken(c)) {
				token = tp = sp;
				midtoken = TRUE;
			}
		}
		if (((level == 0 && !ininit) || number) &&
		    !inquote && !incomm && !Oincomm && !gotmacro) {
			if (midtoken) {
				if (endtoken(c)) {
					int pfline = lineno;
		    			TagType t;
					if (!intypedef && !inextern &&
					    (inclass || inmethod || start_entry(&sp, token))) {
						if (inmethod && !isalpha(*token))
			    				substrcpy(tok,token+1,tp-token);
			    			else
			    				substrcpy(tok,token,tp-token+1);
						getline();
						if (!number) infunc = TRUE;
						if (number) t = MACRO; else
						if (inmethod) t = METHOD; else
						if (inclass) t = CLASS; else
						t = FUNC;
						pfnote(tok, pfline, 1, t);
						if (inmethod || inclass) infunc = TRUE;
						inmethod = inclass = FALSE;
					} else if (!number && !infunc &&
					    !inextern && !inbracket &&
					    !seenparen) {
						Int h;
						char lastc='\0';
						substrcpy(tok,token,tp-token+1);
						if (!intypedef &&
						    (!LastNode || lineno != LastNode->lno))
							getline();
						/*
						 * Skip over closing parens to
						 * deal with  int (*x)();
						 */
						while (iswhite(c) || c == ')' || c=='(') {
							lastc=c;
			    				if (c == '\n') {
								lineno++;
								lineftell =
								    ftell(inf);
							}
							c = getc(inf);
						}
						(void) ungetc(c, inf);
						h = hash(tok);
						if (isstruct(h, tok) ||
						    isenum(h, tok) ||
						    isunion(h, tok)) {
							if (!intypedef)
								startline = pfline;
							instruct = TRUE;
						} else if (istypedef(h, tok)) {
							startline = pfline;
							intypedef = TRUE;
						} else if (isextern(h, tok))
							inextern = TRUE;
						else if (!begtoken(c) &&
						   c != '*' && c != '(') {
							if (instruct && c == '{') {
								if (!NoStruct)
									trynote(tok, startline, STRUCT);
							} else if (intypedef) {
								if (!NoTypedef)
									trynote(tok, startline, TYPEDEF);
				    			} else if (!firstword || lastc==')')
								  if (!NoGlobal)
									trynote(tok, pfline, GLOBAL);
						}
						if (!isstatic(h, tok))
							firstword = FALSE;
					}
					midtoken = FALSE;
					token = sp;
				} else if (intoken(c))
					tp++;
			} else if (begtoken(c)) {
				token = tp = sp;
				midtoken = TRUE;
			}
		}
		if (!incomm && !Oincomm && !inquote && !inchar && !number) switch (c) {
		case ';':
			if (level == 0) {
				if (ininit && LastNode &&
				    LastNode->type == GLOBAL)
					LastNode->length =
					    lineno - LastNode->lno + 1;
				ininit = inextern = FALSE;
				instruct = intypedef = FALSE;
				firstword = TRUE;
			}
			break;
		case '=':
			if (ObjectiveC && level == 0 && sp == line)
	    			inclass = TRUE;
			else if (level == 0 && !inclass)
				ininit = TRUE;
			break;
		case ',':
			if (level == 0 && !inbracket) {
				if (ininit && LastNode &&
				    LastNode->type == GLOBAL)
					LastNode->length =
					    lineno - LastNode->lno + 1;
				ininit = FALSE;
			}
			break;
		case '[': inbracket = TRUE; break;
		case ']': inbracket = FALSE; break;
		}
		sp++;
		if (c == '\n' || sp > &line[sizeof (line) - BUFSIZ]) {
			tp = token = sp = line;
			lineftell = ftell(inf);
			if (PrevLastNode)
				LastNode = PrevLastNode, PrevLastNode = 0;
			number = gotmacro = midtoken = inquote = inchar = FALSE;
		}
	}
	if (MainNode) {
		MainNode->caller = MmainNode->caller;
		MainNode->callee = MmainNode->callee;
		MainNode->ncaller= MmainNode->ncaller;
		MainNode->ncallee= MmainNode->ncallee;
		MainNode->length = MmainNode->length;
	}
}

hash(s)
	Char *s;
{
	Int h = 0;
	while (*s) h += *s++;
	return h;
}

NODE *
getnode(s)
	register char *s;
{
	register NODE *n = findnode(s);
	return n? n : addtag(s, 0, 0, NOTYPE, 1);
}

calls(token)
	char *token;
/*
 * Notes that the current function makes a call to the passed function,
 * by adding them to each others calls and calledby lists.
 */
{
	if (!Ckeyword(token)){
		NODE *n = getnode(token);
		addref(LastNode, token, 0, !n->undecl);
		addref(n, LastNode->entry, 1, 1);
	}
}

addref(n, s, caller, countit)
	register NODE *n;
	Char *s;
{
	register List *r, *t, *prev = NULL;
	Int dif, *length;

	if (caller) {
		t = n->caller;
		length = &n->ncaller;
	} else {
		t = n->callee;
		length = &n->ncallee;
	}
	for (t = caller ? n->caller : n->callee; t; t = t->next) {
		dif = strcmp(t->name, s);
		if (dif == 0) return;
		if (dif > 0) break;
		prev = t;
	}
	if (countit && (++*length > MaxRefs+1))
		return;
	r = alloc(List);
	r->name = savestr(s);
	r->next = t;
	if (prev) prev->next = r;
	else if (caller)
		n->caller = r;
	else
		n->callee = r;
}

start_entry(lp, token)
	char **lp, *token;
/*
 * check to see if current token is at the start of a function or typedef;
 * set seenparen and seenmactoken as a side effect
 */
{

	Char *sp = *lp, c = *sp;
	static char gotdefine = 0;
	char firsttok;	/* T if have seen first token in ()'s */
	int bad;

	seenparen = FALSE;
	if (number) {
		bad = TRUE;
		if (!seenmactoken && token[0] == 'd')
			gotdefine = TRUE;
		else if (gotdefine && (!NoMacro || c=='(')) /*)*/ {
			gotmacro = TRUE;
			bad = FALSE;
			gotdefine = FALSE;	/* reset for next time */
		}
		seenmactoken = TRUE;
		goto ret;
	}
	bad = FALSE;
	while (iswhite(c)) {
		*++sp = c = getc(inf);
		if (c == '\n') {
			lineno++;
			if (sp > &line[sizeof (line) - BUFSIZ])
				goto ret;
		}
	}
	if (c != '(') {
		bad = TRUE;
		goto ret;
	}
	seenparen = TRUE;
	firsttok = FALSE;
	while ((*++sp = c = getc(inf)) != ')') {
		if (c == '\n') {
			lineno++;
			if (sp > &line[sizeof (line) - BUFSIZ])
				goto ret;
		}
		/*
		 * This line used to confuse ctags:
		 *	int	(*oldhup)();
		 * This fixes it. A nonwhite char before the first
		 * token, other than a / (in case of a comment in there)
		 * makes this not a declaration.
		 */
		if (begtoken(c) || c == '/')
			firsttok++;
		else if (!iswhite(c) && !firsttok) {
			bad = TRUE;
			goto ret;
		}
	}
	/*
	 * skip over any extra parenthasess, to be able to tell
	 *	int (*func(parms))();
	 * from
	 *	int (*func(parms)) parms ...
	 */
	while (iswhite(*++sp = c = getc(inf)) || c == '(' || c == ')')
		if (c == '\n') {
			lineno++;
			if (sp > &line[sizeof (line) - BUFSIZ])
				break;
		}
ret:
	*lp = --sp;
	if (c == '\n')
		lineno--;
	(void) ungetc(c,inf);
	return (!bad && isgood(c));
}

Y_entries(){ /* find the yacc tags and put them in */
	Char *sp;
	Int brace = 0;
	char *osp, tok[BUFSIZ], *toss_comment();
	int in_rule = 0, toklen;

	getline();
	pfnote("yyparse", lineno, TRUE, FUNC);
	for (*(sp = line) = '\0';; sp++) {
		if (*sp == '\0') {
			if (fgets(line, sizeof (line), inf) == NULL)
				break;
			sp = line;
		}
		switch (*sp) {

		case '\n': lineno++;
			/* fall thru... */
		case ' ': case '\t': case '\f': case '\r':
			continue;
		case '"':
			do
				while (*++sp != '"')
					;
			while (sp[-1] == '\\');
			continue;
		case '\'':
			do
				while (*++sp != '\'')
					;
			while (sp[-1] == '\\');
			continue;
		case '/':
			if (*++sp == '*')
				sp = toss_comment(sp);
			else
				--sp;
			continue;
		case '{':
			brace++;
			continue;
		case '}':
			brace--;
			continue;
		case '%':
			if (sp[1] == '%' && sp == line)
				return;
			continue;
		case '|': case ';':
			in_rule = FALSE;
			continue;
		}
		if (brace != 0 || in_rule ||
		    (!isalpha(*sp) && *sp != '.' && *sp != '_'))
			continue;
		/* collect yacc non-terminal token */
		osp = sp++;
		while (isalnum(*sp) || *sp == '_' || *sp == '.')
			sp++;
		toklen = sp - osp;
		while (isspace(*sp) && *sp != '\n')
			sp++;
		if (*sp != ':' &&
		    (*sp != '\0' || first_char() != ':')) {
			sp--;
			continue;
		}
		strncpy(tok, osp, toklen);
		tok[toklen] = '\0';
		strcpy(lbuf, line);
		lbuf[strlen(lbuf) - 1] = '\0';
		pfnote(tok, lineno, FALSE, YACCTOKEN);
		in_rule = TRUE;
	}
}

char *
toss_comment(cp)
	Char *cp;
{
	Char *tp;
	for (;;) {
		tp=index(cp, '*');
		if (!tp) {
			if (index(cp, '\0')[-1] == '\n')
				lineno++;
			if (fgets(line, sizeof (line), inf) == NULL)
				break;
			cp = line;
			continue;
		}
		if (*++tp == '/')
			return (tp);
		cp = tp;
	}
	line[0] = '\0';		/* force EOF to be recognized */
	return (line);
}

getline(){
	long saveftell = ftell(inf);
	Char *cp;
	fseek(inf, lineftell, 0);
	fgets(lbuf, sizeof (lbuf), inf);
	if (cp = rindex(lbuf, '\n'))
		*cp = '\0';
	fseek(inf, saveftell, 0);
}

free_tree(n)
	NODE *n;
{
	while (n) {
		free_tree(n->right);
		cfree(n);
		n = n->left;
	}
}

add_node(n, current)
	NODE *n, *current;
{
	int dif = strcmp(n->entry, current->entry);
	if (dif < 0) {
		if (current->left)
			add_node(n, current->left);
		else
			current->left = n;
		return;
	}
	if (dif > 0){
		if (current->right)
			add_node(n, current->right);
		else
			current->right = n;
		return;
	}
 	if (dif==0 && (!NoDuplicates || ObjectiveC) && strcmp(n->file,current->file)) {
		if (current->right)
			add_node(n, current->right);
		else
			current->right = n;
		return;
	}

	/*
	 * Replace duplicate node in tree if it's undeclared
	 * or a struct replacing a typedef or a non-typedef
	 * replacing a struct.
	 * Or a struct/typedef replacing a macro.
	 */
	if (current->undecl ||
	    (current->type == TYPEDEF && n->type == STRUCT) ||
	    (current->type == STRUCT && n->type != TYPEDEF) ||
	    (current->type == MACRO && n->type == STRUCT) ||
	    (current->type == MACRO && n->type == TYPEDEF)) {
		/*
		 * Keep the caller/caller lists and
		 * tree links of the old node.
		 */
		n->caller = current->caller;
		n->callee = current->callee;
		n->left = current->left;
		n->right = current->right;
		/* free the old node's name */
		free(current->entry);
		/* take everything from the new node */
		*current = *n;
		/* clean up */
		if (LastNode == n)
			LastNode = current;
		free((char *)n);
		return;
	}
	if (n->type == STRUCT ||
	    (n->type == TYPEDEF && current->type == STRUCT) ||
	    (n->type == GLOBAL && !strcmp(n->entry, "sccsid"))) {
		/*
		 * Ignore duplicate new node if it's a struct
		 * or a typedef which duplicates a struct
		 * or a global named "sccsid".
		 */
		free(n->entry);
		free((char *)n);
		if (LastNode == n) LastNode = NULL;
		return;
	}
	if (NoWarn) return;
	if (n->file == current->file)
	    return e("duplicate: %s (%s@%d)", n->entry, n->file, lineno);
	if (!current->been_warned) {
	    current->been_warned = TRUE;
	    return e("duplicate: %s (%s %s)",n->entry,n->file,current->file);
	}
}

put_refs(n)
	register NODE *n;
{
	register List *l;
	Int i;

	fputs(" { ", outf);
	for (l = n->caller, i = 0; l && i < MaxRefs; l = l->next)
		if (lookup(l->name)) {
			i++;
			fprintf(outf, "%s ", l->name);
		}
	if (l) fputs("(etc) ", outf);
	fputs("}\t{ ", outf);
	for (l = n->callee, i = 0; l && i < MaxRefs; l = l->next)
		if (lookup(l->name)) {
			i++;
			fprintf(outf, "%s ", l->name);
		}
	if (l) fputs("(etc) ", outf);
	fputs("}", outf);
}

put_entries(n)
	register NODE *n;
{
	Char *sp;
	if (!n) return;
	put_entries(n->left);
	if (!n->undecl) {
		if (!cxref) {
			if (n->f) {  /* a function */
				fprintf(outf, "%s\t\t%s\t%c^",
				    n->entry, n->file, searchar);
				for (sp = n->pat; *sp; sp++)
					if (*sp == '\\')
						fprintf(outf, "\\\\");
					else if (*sp == searchar)
						fprintf(outf, "\\%c", searchar);
					else
						putc(*sp, outf);
				fprintf(outf, "%c", searchar);
			} else		/* text pattern inadequate */
				fprintf(outf, "%s\t\t%s\t%d",
				    n->entry, n->file, n->lno);
	    			if (!NoXrefs) {
					fprintf(outf, " ;\" %s %d",
						Typename[(int)n->type],
						n->length);
					put_refs(n);
	    			}
				fputs("\n", outf);
		} else if (vgrind)
			fprintf(stdout, "%s %s %d\n",
			    n->entry, n->file, (n->lno+63)/64);
		else
			fprintf(stdout, "%-16s%4d %-16s %s\n",
			    n->entry, n->lno, n->file, n->pat);
	}
	put_entries(n->right);
}

char	*dbp = lbuf;
int	pfcnt;

PF_funcs(f)
	FILE *f;
{

	lineno = 0;
	pfcnt = 0;
	while (fgets(lbuf, sizeof (lbuf), f)) {
		lineno++;
		dbp = lbuf;
		if (*dbp == '%')
			dbp++;	/* Ratfor escape to fortran */
		while (isspace(*dbp))
			dbp++;
		if (*dbp == 0)
			continue;
		switch (*dbp | ' ') {

		case 'i':
			if (tail("integer"))
				takeprec();
			break;
		case 'r':
			if (tail("real"))
				takeprec();
			break;
		case 'l':
			if (tail("char"))
				takeprec();
			break;
		case 'c':
			if (tail("complex") || tail("character"))
				takeprec();
			break;
		case 'd':
			if (tail("double")) {
				while (isspace(*dbp))
					dbp++;
				if (*dbp == 0)
					continue;
				if (tail("precision"))
					break;
				continue;
			}
			break;
		}
		while (isspace(*dbp))
			dbp++;
		if (*dbp == 0)
			continue;
		switch (*dbp|' ') {

		case 'f':
			if (tail("function"))
				getit();
			continue;
		case 's':
			if (tail("subroutine"))
				getit();
			continue;
		case 'p':
			if (tail("program")) {
				getit();
			continue;
			}
			if (tail("procedure"))
				getit();
			continue;
		}
	}
	return (pfcnt);
}

tail(cp)
	char *cp;
{
	Int len = 0;

	while (*cp && (*cp&~' ') == ((*(dbp+len))&~' '))
		cp++, len++;
	if (*cp == 0) {
		dbp += len;
		return (1);
	}
	return (0);
}

takeprec(){
	while (isspace(*dbp))
		dbp++;
	if (*dbp != '*')
		return;
	dbp++;
	while (isspace(*dbp))
		dbp++;
	if (!isdigit(*dbp)) {
		--dbp;		/* force failure */
		return;
	}
	do
		dbp++;
	while (isdigit(*dbp));
}

getit(){
	Char *cp;
	char c, nambuf[BUFSIZ];

	for (cp = lbuf; *cp; cp++)
		;
	*--cp = '\0';	/* zap newline */
	while (isspace(*dbp))
		dbp++;
	if (*dbp == 0 || !isalpha(*dbp))
		return;
	for (cp = dbp+1; *cp && (isalpha(*cp) || isdigit(*cp)); cp++)
		;
	c = cp[0];
	cp[0] = '\0';
	(void) strcpy(nambuf, dbp);
	cp[0] = c;
	pfnote(nambuf, lineno, TRUE, FUNC);
	pfcnt++;
}

char *
savestr(cp)
	Char *cp;
{
	return (strcpy(malloc(strlen(cp)+1), cp));
}

/* lisp tag functions just look for (def or (DEF */

L_funcs (f)
FILE *f;
{
	Int special;

	lineno = 0;
	pfcnt = 0;
	while (fgets(lbuf, sizeof (lbuf), f)) {
		lineno++;
		dbp = lbuf;
		if (dbp[0] == '(' && 
		    (dbp[1] == 'D' || dbp[1] == 'd') &&
		    (dbp[2] == 'E' || dbp[2] == 'e') &&
		    (dbp[3] == 'F' || dbp[3] == 'f')) {
			dbp += 4;
			if (striccmp(dbp, "method") == 0 ||
			    striccmp(dbp, "wrapper") == 0 ||
			    striccmp(dbp, "whopper") == 0)
				special = TRUE;
			else
				special = FALSE;
			while (!isspace(*dbp))
				dbp++;
			while (isspace(*dbp))
				dbp++;
			L_getit(special);
		}
	}
}

L_getit(special)
	int special;
{
	register char *cp;
	char c, nambuf[BUFSIZ];

	for (cp = lbuf; *cp; cp++)
		;
	*--cp = '\0';		/* zap newline */
	if (*dbp == '\0')
		return;
	if (special) {
		cp = index(dbp, ')');
		if (cp == NULL)
			return;
		while (cp >= dbp && *cp != ':')
			cp--;
		if (cp < dbp)
			return;
		dbp = cp;
		while (*cp && *cp != ')' && *cp != ' ')
			cp++;
	} else
		for (cp = dbp+1; *cp && *cp != '(' && *cp != ' '; cp++)
			;
	c = cp[0];
	cp[0] = '\0';
	(void) strcpy(nambuf, dbp);
	cp[0] = c;
	pfnote(nambuf, lineno,TRUE, FUNC);
	pfcnt++;
}

striccmp(s,p)
	Char *s,*p;
/*
 * Compare two strings over the length of the second, ignoring case.
 * If same, return 0.
 * If different, return the difference of the first two different characters.
 * The pattern (second string) is assumed to be completely lower case.
 */
{
	Int c;
	while (*p) {
		c = isupper(*s)? tolower(*s) : *s;
		if (c != *p) return c - *p;
		p++, s++;
	}
	return 0;
}

first_char()
/*
 * Return the first non-blank character starting from the
 * current offset in the file.  After finding such, rewind
 * the input file to the previous offset.
 */
{
	Int c;
	register long off;
	off = ftell(inf);
	while ((c = getc(inf)) != EOF)
		if (!isspace(c) && c != '\r') break;
	fseek(inf, off, 0);
	return (EOF);
}

toss_yysec(){ /* toss away code until the next "%%" line */
	char s[BUFSIZ];
	do {
		lineftell = ftell(inf);
		if (!fgets(s,sizeof s,inf)) break;
		lineno++;
	} while (strncmp(s,"%%",2));
}

init()
/*
 * This routine sets up the boolean psuedo-functions which work
 * by seting boolean flags dependent upon the corresponding character
 * Every char which is NOT in that string is not a white char.  Therefore,
 * all of the array "_wht" is set to FALSE, and then the elements
 * subscripted by the chars in "white" are set to TRUE.  Thus "_wht"
 * of a char is TRUE if it is the string "white", else FALSE.
 *
 * Also computes and saves hash value for Ckwords.
 */
{
	Char *s;
	Int i;
	register struct keyword *p;

	for (i=0; i<0177; i++) {
		_wht[i] = _etk[i] = _itk[i] = _btk[i] = FALSE;
		_gd[i] = TRUE;
	}
	/* white-space/end-of-token chars */
	for (s=" \f\t\n"; *s; s++) _wht[*s] = _etk[*s] = TRUE;
	/* additional end-of-token chars */
	for (s = "\"'#()[]{}=-+%*/&|^~!<>;,.:?"; *s; s++) _etk[*s] = TRUE;
	/* begin-token/in-token chars */
	s = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
	for (; *s; s++) _btk[*s] = _itk[*s] = TRUE;
	/* addition in-token chars */
	for (s = "0123456789";  *s; s++) _itk[*s] = TRUE;
	/* non-valid after-function chars */
	for (s = ",;"; *s; s++) _gd[*s]  = FALSE;
	for (p = Ckword; p->name; p++) {
		Int h;
		p->hash = hash(p->name);
		for (h = p->hash; CkwordHash[h % NCKEYHASH]; h <<= 1)
			;
		CkwordHash[h % NCKEYHASH] = p;
	}
}

e(a,b,c,d){ fprintf(stderr,a,b,c,d); fprintf(stderr,"\n"); }

use(){
   e("use: %s [-abdgmrstuwvx] [-f infile] [-o tagfile] [-n #] files",av0);
   e(" -a         append to tags file");
   e(" -b         write backward search patterns ?...? instead of /.../");
   e(" -d         create entries for duplicate declarations");
   e(" -g         suppress indexing of global variables");
   e(" -m                  ...         parameterless #define macros");
   e(" -r                  ...         callee/caller reference lists");
   e(" -s                  ...         structs, unions, and enums");
   e(" -t                  ...         typedefs");
   e(" -u         update tags file (almost always faster to rebuild)");
   e(" -v         write index for vgrind(1) on stdout");
   e(" -w         suppress warning diagnostics");
   e(" -x         print a simplified list for humans");
   e(" -f infile  read filenames from 'infile'");
   e(" -o tagfile write output to 'tagfile' ('tags' by default)");
   e(" -n #       set maximum number of references to # (%d)",MaxRefs);
   exit(1);
}

main(ac,av) char *av[]; {
	char s[512];
	int i,n;
	char *in = (char *)0;

	for_each_argument {
	Case 'a': append = 1;
	Case 'b': searchar = '?';
	Case 'd': NoDuplicates = 0;
	Case 'g': NoGlobal = 1;
	Case 'm': NoMacro = 1;
	Case 'r': NoXrefs = 1;
	Case 's': NoStruct = 1;
	Case 't': NoTypedef = 1;
	Case 'u': update = 1;
	Case 'v': vgrind=cxref=NoStruct=NoTypedef=NoXrefs=NoGlobal=NoMacro=1;
	Case 'w': NoWarn = 1;
	Case 'x': cxref=NoStruct=NoTypedef=NoXrefs=NoGlobal=NoMacro=1;
	Case 'f': in = argument;
	Case 'o': outfile = argument;
	Case 'n': MaxRefs = atoi(argument);
	Default:  use();
	}

	if (i==ac && !in) use();

	n=i;
	init();

	if (in){ /* read filenames from file 'in' */
		FILE *f = fopen(in,"r");
		if (f){
			while (fscanf(f,"%s",s)>0 && *s){
				find_entries(s);
				*s = '\0';
			}
			fclose(f);
		} else
			e("%s: warning, couldn't read files from '%s'",av0,in);
	}

	while (i<ac) find_entries(av[i++]);
	if (cxref) put_entries(head), exit(0);
	if (update && !in) {
		i=n;
		while(i<ac)
			system(sprintf(s,
			    "mv %s OTAGS;fgrep -v '\t%s\t' OTAGS >%s;rm OTAGS",
				outfile, av[i++], outfile));
		append++;
	}
	outf = fopen(outfile, append ? "a" : "w");
	if (!outf) perror(outfile), exit(1);
	put_entries(head);
	fclose(outf);
	if (update && !in) system(sprintf(s,"sort %s -o %s", outfile, outfile));
	exit(0);
}
