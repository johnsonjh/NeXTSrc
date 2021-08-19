# include "stdio.h"
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern char yytext[];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;
FILE *yyin ={stdin}, *yyout ={stdout};
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
# define A 2
# define str 4
# define chc 6
# define sc 8
# define reg 10
# define comment 12
#include	"awk.h"
#include	"awk.def"
#undef	input	/* defeat lex */
extern int	yylval;
extern int	mustfld;
extern int	ldbg;

int	lineno	= 1;
#define	RETURN(x)	{if (ldbg) ptoken(x); return(x); }
#define	CADD	cbuf[clen++]=yytext[0]; if(clen>=CBUFLEN-1) {yyerror("string too long", cbuf); BEGIN A;}
#define	CBUFLEN	150
char	cbuf[CBUFLEN];
int	clen, cflag;
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
switch (yybgin-yysvec-1) {	/* witchcraft */
	case 0:
		BEGIN A;
		break;
	case sc:
		BEGIN A;
		RETURN('}');
	}
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
	lineno++;
break;
case 2:
lineno++;
break;
case 3:
	;
break;
case 4:
lineno++;
break;
case 5:
	RETURN(BOR);
break;
case 6:
RETURN(XBEGIN);
break;
case 7:
	RETURN(XEND);
break;
case 8:
RETURN(EOF);
break;
case 9:
	RETURN(AND);
break;
case 10:
	RETURN(NOT);
break;
case 11:
	{ yylval = NE; RETURN(RELOP); }
break;
case 12:
	{ yylval = MATCH; RETURN(MATCHOP); }
break;
case 13:
	{ yylval = NOTMATCH; RETURN(MATCHOP); }
break;
case 14:
	{ yylval = LT; RETURN(RELOP); }
break;
case 15:
	{ yylval = LE; RETURN(RELOP); }
break;
case 16:
	{ yylval = EQ; RETURN(RELOP); }
break;
case 17:
	{ yylval = GE; RETURN(RELOP); }
break;
case 18:
	{ yylval = GT; RETURN(RELOP); }
break;
case 19:
	{ yylval = APPEND; RETURN(RELOP); }
break;
case 20:
	{ yylval = INCR; RETURN(INCR); }
break;
case 21:
	{ yylval = DECR; RETURN(DECR); }
break;
case 22:
	{ yylval = ADDEQ; RETURN(ASGNOP); }
break;
case 23:
	{ yylval = SUBEQ; RETURN(ASGNOP); }
break;
case 24:
	{ yylval = MULTEQ; RETURN(ASGNOP); }
break;
case 25:
	{ yylval = DIVEQ; RETURN(ASGNOP); }
break;
case 26:
	{ yylval = MODEQ; RETURN(ASGNOP); }
break;
case 27:
	{ yylval = ASSIGN; RETURN(ASGNOP); }
break;
case 28:
{	if (atoi(yytext+1)==0) {
				yylval = (hack)lookup("$record", symtab, 0);
				RETURN(STRING);
			} else {
				yylval = fieldadr(atoi(yytext+1));
				RETURN(FIELD);
			}
		}
break;
case 29:
{ RETURN(INDIRECT); }
break;
case 30:
	{ mustfld=1; yylval = (hack)setsymtab(yytext, EMPTY, 0.0, NUM, symtab); RETURN(VAR); }
break;
case 31:
{
		yylval = (hack)setsymtab(yytext, EMPTY, atof(yytext), CON|NUM, symtab); RETURN(NUMBER); }
break;
case 32:
{ BEGIN sc; lineno++; RETURN(';'); }
break;
case 33:
	{ BEGIN sc; RETURN(';'); }
break;
case 34:
	{ lineno++; RETURN(';'); }
break;
case 35:
	{ lineno++; RETURN(NL); }
break;
case 36:
RETURN(WHILE);
break;
case 37:
	RETURN(FOR);
break;
case 38:
	RETURN(IF);
break;
case 39:
	RETURN(ELSE);
break;
case 40:
	RETURN(NEXT);
break;
case 41:
	RETURN(EXIT);
break;
case 42:
RETURN(BREAK);
break;
case 43:
RETURN(CONTINUE);
break;
case 44:
{ yylval = PRINT; RETURN(PRINT); }
break;
case 45:
{ yylval = PRINTF; RETURN(PRINTF); }
break;
case 46:
{ yylval = SPRINTF; RETURN(SPRINTF); }
break;
case 47:
{ yylval = SPLIT; RETURN(SPLIT); }
break;
case 48:
RETURN(SUBSTR);
break;
case 49:
RETURN(INDEX);
break;
case 50:
	RETURN(IN);
break;
case 51:
RETURN(GETLINE);
break;
case 52:
{ yylval = FLENGTH; RETURN(FNCN); }
break;
case 53:
	{ yylval = FLOG; RETURN(FNCN); }
break;
case 54:
	{ yylval = FINT; RETURN(FNCN); }
break;
case 55:
	{ yylval = FEXP; RETURN(FNCN); }
break;
case 56:
	{ yylval = FSQRT; RETURN(FNCN); }
break;
case 57:
{ yylval = (hack)setsymtab(yytext, tostring(""), 0.0, STR|NUM, symtab); RETURN(VAR); }
break;
case 58:
	{ BEGIN str; clen=0; }
break;
case 59:
	{ BEGIN comment; }
break;
case 60:
{ BEGIN A; lineno++; RETURN(NL); }
break;
case 61:
;
break;
case 62:
	{ yylval = yytext[0]; RETURN(yytext[0]); }
break;
case 63:
{ BEGIN chc; clen=0; cflag=0; }
break;
case 64:
{ BEGIN chc; clen=0; cflag=1; }
break;
case 65:
RETURN(QUEST);
break;
case 66:
RETURN(PLUS);
break;
case 67:
RETURN(STAR);
break;
case 68:
RETURN(OR);
break;
case 69:
RETURN(DOT);
break;
case 70:
RETURN('(');
break;
case 71:
RETURN(')');
break;
case 72:
RETURN('^');
break;
case 73:
RETURN('$');
break;
case 74:
{ sscanf(yytext+1, "%o", &yylval); RETURN(CHAR); }
break;
case 75:
{	if (yytext[1]=='n') yylval = '\n';
			else if (yytext[1] == 't') yylval = '\t';
			else yylval = yytext[1];
			RETURN(CHAR);
		}
break;
case 76:
{ BEGIN A; unput('/'); }
break;
case 77:
	{ yyerror("newline in regular expression"); lineno++; BEGIN A; }
break;
case 78:
	{ yylval = yytext[0]; RETURN(CHAR); }
break;
case 79:
	{ char *s; BEGIN A; cbuf[clen]=0; s = tostring(cbuf);
		cbuf[clen] = ' '; cbuf[++clen] = 0;
		yylval = (hack)setsymtab(cbuf, s, 0.0, CON|STR, symtab); RETURN(STRING); }
break;
case 80:
	{ yyerror("newline in string"); lineno++; BEGIN A; }
break;
case 81:
{ cbuf[clen++]='"'; }
break;
case 82:
{ cbuf[clen++]='\n'; }
break;
case 83:
{ cbuf[clen++]='\t'; }
break;
case 84:
{ cbuf[clen++]='\\'; }
break;
case 85:
	{ CADD; }
break;
case 86:
{ cbuf[clen++]=']'; }
break;
case 87:
{ BEGIN reg; cbuf[clen]=0; yylval = (hack)tostring(cbuf);
		if (cflag==0) { RETURN(CCL); }
		else { RETURN(NCCL); } }
break;
case 88:
	{ yyerror("newline in character class"); lineno++; BEGIN A; }
break;
case 89:
	{ CADD; }
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */

input()
{
	register c;
	extern char *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr);
	else if (yyin == NULL)
		c = *lexprog++;
	else
		c = getc(yyin);
	if (c == '\n')
		yylineno++;
	else if (c == EOF)
		c = 0;
	return(c);
}

startreg()
{
	BEGIN reg;
}

ptoken(n)
{
	extern struct tok {
		char *tnm;
		int yval;
	} tok[];
	extern char yytext[];
	extern int yylval;

	printf("lex:");
	if (n < 128) {
		printf(" %c\n",n);
		return;
	}
	if (n <= 256 || n >= LASTTOKEN) {
		printf("? %o\n",n);
		return;
	}
	printf(" %s",tok[n-257].tnm);
	switch (n) {

	case RELOP:
	case MATCHOP:
	case ASGNOP:
	case STRING:
	case FIELD:
	case VAR:
	case NUMBER:
	case FNCN:
		printf(" (%s)", yytext);
		break;

	case CHAR:
		printf(" (%o)", yylval);
		break;
	}
	putchar('\n');
}
int yyvstop[] ={
0,

62,
0,

3,
62,
0,

35,
0,

10,
62,
0,

58,
62,
0,

59,
62,
0,

29,
62,
0,

62,
0,

62,
0,

62,
0,

62,
0,

62,
0,

62,
0,

62,
0,

31,
62,
0,

62,
0,

14,
62,
0,

27,
62,
0,

18,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

57,
62,
0,

62,
0,

33,
62,
0,

12,
62,
0,

3,
62,
0,

1,
35,
0,

59,
62,
0,

85,
0,

80,
0,

79,
85,
0,

85,
0,

89,
0,

88,
0,

89,
0,

87,
89,
0,

78,
0,

77,
0,

73,
78,
0,

70,
78,
0,

71,
78,
0,

67,
78,
0,

66,
78,
0,

69,
78,
0,

76,
78,
0,

65,
78,
0,

63,
78,
0,

78,
0,

72,
78,
0,

68,
78,
0,

61,
0,

60,
0,

11,
0,

13,
0,

29,
0,

28,
0,

26,
0,

9,
0,

24,
0,

20,
0,

22,
0,

21,
0,

23,
0,

31,
0,

25,
0,

31,
0,

31,
0,

34,
0,

15,
0,

16,
0,

17,
0,

19,
0,

57,
0,

57,
0,

57,
0,

30,
57,
0,

57,
0,

4,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

38,
57,
0,

50,
57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

5,
0,

32,
0,

2,
0,

81,
0,

84,
0,

82,
0,

83,
0,

86,
0,

64,
0,

75,
0,

75,
0,

31,
0,

57,
0,

7,
57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

55,
57,
0,

37,
57,
0,

57,
0,

57,
0,

54,
57,
0,

57,
0,

53,
57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

57,
0,

39,
57,
0,

41,
57,
0,

57,
0,

57,
0,

57,
0,

40,
57,
0,

57,
0,

57,
0,

57,
0,

56,
57,
0,

57,
0,

57,
0,

74,
0,

6,
57,
0,

57,
0,

42,
57,
0,

57,
0,

57,
0,

49,
57,
0,

57,
0,

44,
57,
0,

47,
57,
0,

57,
0,

57,
0,

36,
57,
0,

57,
0,

57,
0,

57,
0,

52,
57,
0,

45,
57,
0,

57,
0,

48,
57,
0,

8,
57,
0,

57,
0,

51,
57,
0,

46,
57,
0,

43,
57,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] ={
0,0,	0,0,	3,15,	0,0,	
7,61,	0,0,	0,0,	5,57,	
0,0,	13,79,	3,16,	3,17,	
7,61,	7,62,	30,97,	5,57,	
5,58,	13,79,	13,80,	39,107,	
0,0,	0,0,	52,125,	52,126,	
0,0,	0,0,	0,0,	54,127,	
0,0,	0,0,	0,0,	0,0,	
83,83,	0,0,	3,18,	3,19,	
3,20,	3,21,	3,22,	3,23,	
5,59,	23,86,	0,0,	3,24,	
3,25,	52,125,	3,26,	3,27,	
3,28,	3,29,	54,127,	7,61,	
6,59,	54,128,	5,57,	83,83,	
13,79,	25,88,	26,90,	0,0,	
3,30,	3,31,	3,32,	3,33,	
18,81,	22,85,	3,34,	3,35,	
7,61,	24,87,	3,36,	5,57,	
28,93,	13,79,	26,91,	25,89,	
31,98,	32,99,	35,103,	3,37,	
36,104,	3,38,	27,92,	27,92,	
27,92,	27,92,	27,92,	27,92,	
27,92,	27,92,	27,92,	27,92,	
37,105,	3,39,	38,106,	7,63,	
7,64,	103,140,	5,60,	3,40,	
3,41,	104,141,	3,42,	3,43,	
3,44,	44,113,	3,45,	4,54,	
4,55,	3,46,	6,60,	3,47,	
41,109,	3,48,	8,63,	8,64,	
3,49,	33,100,	33,101,	40,108,	
3,50,	43,112,	47,118,	48,119,	
50,123,	3,51,	3,52,	3,53,	
51,124,	18,82,	42,110,	4,18,	
4,19,	4,56,	4,21,	4,22,	
4,23,	75,135,	45,114,	46,116,	
4,24,	4,25,	42,111,	4,26,	
4,27,	4,28,	45,115,	11,65,	
106,142,	46,117,	63,131,	63,134,	
92,96,	108,143,	109,144,	11,65,	
11,66,	4,30,	4,31,	4,32,	
4,33,	49,120,	49,121,	110,145,	
4,35,	112,148,	49,122,	4,36,	
63,132,	113,149,	111,146,	116,152,	
56,128,	117,153,	63,133,	115,150,	
4,37,	111,147,	4,38,	118,154,	
56,128,	56,129,	11,67,	119,155,	
92,96,	60,130,	11,68,	11,69,	
11,70,	11,71,	4,39,	115,151,	
11,72,	11,73,	11,65,	121,158,	
4,40,	4,41,	120,156,	4,42,	
4,43,	4,44,	122,159,	4,45,	
120,157,	123,160,	4,46,	140,162,	
4,47,	11,74,	4,48,	11,65,	
21,83,	4,49,	142,163,	143,164,	
144,165,	4,50,	145,166,	56,128,	
146,167,	12,67,	4,51,	4,52,	
4,53,	12,68,	12,69,	12,70,	
12,71,	149,168,	150,169,	12,72,	
12,73,	152,170,	154,171,	21,83,	
56,128,	11,75,	11,76,	155,172,	
11,77,	156,173,	157,174,	60,131,	
158,175,	159,176,	160,177,	162,179,	
12,74,	163,180,	164,181,	21,84,	
21,84,	21,84,	21,84,	21,84,	
21,84,	21,84,	21,84,	21,84,	
21,84,	60,132,	165,182,	76,136,	
168,183,	169,184,	170,185,	60,133,	
172,186,	173,187,	11,78,	76,136,	
76,107,	174,188,	176,189,	177,190,	
12,75,	12,76,	29,94,	12,77,	
29,95,	29,95,	29,95,	29,95,	
29,95,	29,95,	29,95,	29,95,	
29,95,	29,95,	84,84,	84,84,	
84,84,	84,84,	84,84,	84,84,	
84,84,	84,84,	84,84,	84,84,	
180,191,	29,96,	182,192,	183,193,	
185,194,	186,195,	188,196,	189,197,	
191,198,	12,78,	76,137,	192,199,	
193,200,	196,201,	199,202,	0,0,	
0,0,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	76,136,	
0,0,	0,0,	0,0,	0,0,	
0,0,	29,96,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
0,0,	0,0,	0,0,	0,0,	
34,102,	0,0,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
34,102,	34,102,	34,102,	34,102,	
94,94,	94,94,	94,94,	94,94,	
94,94,	94,94,	94,94,	94,94,	
94,94,	94,94,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	96,138,	0,0,	96,138,	
0,0,	94,96,	96,139,	96,139,	
96,139,	96,139,	96,139,	96,139,	
96,139,	96,139,	96,139,	96,139,	
137,161,	137,161,	137,161,	137,161,	
137,161,	137,161,	137,161,	137,161,	
137,161,	137,161,	138,139,	138,139,	
138,139,	138,139,	138,139,	138,139,	
138,139,	138,139,	138,139,	138,139,	
0,0,	94,96,	161,178,	161,178,	
161,178,	161,178,	161,178,	161,178,	
161,178,	161,178,	161,178,	161,178,	
0,0};
struct yysvf yysvec[] ={
0,	0,	0,
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-1,	0,		0,	
yycrank+-98,	yysvec+3,	0,	
yycrank+-6,	0,		0,	
yycrank+-18,	yysvec+5,	0,	
yycrank+-3,	0,		0,	
yycrank+-22,	yysvec+7,	0,	
yycrank+0,	0,		0,	
yycrank+0,	0,		0,	
yycrank+-146,	0,		0,	
yycrank+-185,	yysvec+11,	0,	
yycrank+-8,	0,		0,	
yycrank+0,	yysvec+13,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+3,
yycrank+0,	0,		yyvstop+6,
yycrank+3,	0,		yyvstop+8,
yycrank+0,	0,		yyvstop+11,
yycrank+0,	0,		yyvstop+14,
yycrank+203,	0,		yyvstop+17,
yycrank+4,	0,		yyvstop+20,
yycrank+3,	0,		yyvstop+22,
yycrank+8,	0,		yyvstop+24,
yycrank+14,	0,		yyvstop+26,
yycrank+13,	0,		yyvstop+28,
yycrank+34,	0,		yyvstop+30,
yycrank+11,	0,		yyvstop+32,
yycrank+232,	0,		yyvstop+34,
yycrank+4,	0,		yyvstop+37,
yycrank+15,	0,		yyvstop+39,
yycrank+16,	0,		yyvstop+42,
yycrank+56,	0,		yyvstop+45,
yycrank+269,	0,		yyvstop+48,
yycrank+9,	yysvec+34,	yyvstop+51,
yycrank+2,	yysvec+34,	yyvstop+54,
yycrank+22,	yysvec+34,	yyvstop+57,
yycrank+12,	yysvec+34,	yyvstop+60,
yycrank+9,	0,		yyvstop+63,
yycrank+5,	yysvec+34,	yyvstop+65,
yycrank+1,	yysvec+34,	yyvstop+68,
yycrank+22,	yysvec+34,	yyvstop+71,
yycrank+10,	yysvec+34,	yyvstop+74,
yycrank+4,	yysvec+34,	yyvstop+77,
yycrank+36,	yysvec+34,	yyvstop+80,
yycrank+38,	yysvec+34,	yyvstop+83,
yycrank+21,	yysvec+34,	yyvstop+86,
yycrank+9,	yysvec+34,	yyvstop+89,
yycrank+49,	yysvec+34,	yyvstop+92,
yycrank+20,	yysvec+34,	yyvstop+95,
yycrank+4,	0,		yyvstop+98,
yycrank+13,	0,		yyvstop+100,
yycrank+0,	0,		yyvstop+103,
yycrank+18,	0,		yyvstop+106,
yycrank+0,	0,		yyvstop+109,
yycrank+-171,	0,		yyvstop+112,
yycrank+0,	0,		yyvstop+115,
yycrank+0,	0,		yyvstop+117,
yycrank+0,	0,		yyvstop+119,
yycrank+151,	0,		yyvstop+122,
yycrank+0,	0,		yyvstop+124,
yycrank+0,	0,		yyvstop+126,
yycrank+58,	0,		yyvstop+128,
yycrank+0,	0,		yyvstop+130,
yycrank+0,	0,		yyvstop+133,
yycrank+0,	0,		yyvstop+135,
yycrank+0,	0,		yyvstop+137,
yycrank+0,	0,		yyvstop+140,
yycrank+0,	0,		yyvstop+143,
yycrank+0,	0,		yyvstop+146,
yycrank+0,	0,		yyvstop+149,
yycrank+0,	0,		yyvstop+152,
yycrank+0,	0,		yyvstop+155,
yycrank+0,	0,		yyvstop+158,
yycrank+43,	0,		yyvstop+161,
yycrank+-262,	0,		yyvstop+164,
yycrank+0,	0,		yyvstop+166,
yycrank+0,	0,		yyvstop+169,
yycrank+0,	0,		yyvstop+172,
yycrank+0,	0,		yyvstop+174,
yycrank+0,	0,		yyvstop+176,
yycrank+0,	0,		yyvstop+178,
yycrank+23,	0,		yyvstop+180,
yycrank+242,	0,		yyvstop+182,
yycrank+0,	0,		yyvstop+184,
yycrank+0,	0,		yyvstop+186,
yycrank+0,	0,		yyvstop+188,
yycrank+0,	0,		yyvstop+190,
yycrank+0,	0,		yyvstop+192,
yycrank+0,	0,		yyvstop+194,
yycrank+0,	0,		yyvstop+196,
yycrank+83,	yysvec+27,	yyvstop+198,
yycrank+0,	0,		yyvstop+200,
yycrank+344,	0,		yyvstop+202,
yycrank+0,	yysvec+29,	yyvstop+204,
yycrank+366,	0,		0,	
yycrank+0,	0,		yyvstop+206,
yycrank+0,	0,		yyvstop+208,
yycrank+0,	0,		yyvstop+210,
yycrank+0,	0,		yyvstop+212,
yycrank+0,	0,		yyvstop+214,
yycrank+0,	yysvec+34,	yyvstop+216,
yycrank+26,	yysvec+34,	yyvstop+218,
yycrank+33,	yysvec+34,	yyvstop+220,
yycrank+0,	yysvec+34,	yyvstop+222,
yycrank+69,	yysvec+34,	yyvstop+225,
yycrank+0,	0,		yyvstop+227,
yycrank+52,	yysvec+34,	yyvstop+229,
yycrank+44,	yysvec+34,	yyvstop+231,
yycrank+48,	yysvec+34,	yyvstop+233,
yycrank+65,	yysvec+34,	yyvstop+235,
yycrank+51,	yysvec+34,	yyvstop+237,
yycrank+53,	yysvec+34,	yyvstop+239,
yycrank+0,	yysvec+34,	yyvstop+241,
yycrank+75,	yysvec+34,	yyvstop+244,
yycrank+61,	yysvec+34,	yyvstop+247,
yycrank+70,	yysvec+34,	yyvstop+249,
yycrank+59,	yysvec+34,	yyvstop+251,
yycrank+78,	yysvec+34,	yyvstop+253,
yycrank+90,	yysvec+34,	yyvstop+255,
yycrank+81,	yysvec+34,	yyvstop+257,
yycrank+104,	yysvec+34,	yyvstop+259,
yycrank+100,	yysvec+34,	yyvstop+261,
yycrank+0,	0,		yyvstop+263,
yycrank+0,	yysvec+52,	0,	
yycrank+0,	0,		yyvstop+265,
yycrank+0,	yysvec+54,	0,	
yycrank+0,	yysvec+56,	0,	
yycrank+0,	0,		yyvstop+267,
yycrank+0,	0,		yyvstop+269,
yycrank+0,	0,		yyvstop+271,
yycrank+0,	0,		yyvstop+273,
yycrank+0,	0,		yyvstop+275,
yycrank+0,	0,		yyvstop+277,
yycrank+0,	0,		yyvstop+279,
yycrank+0,	0,		yyvstop+281,
yycrank+376,	0,		yyvstop+283,
yycrank+386,	0,		0,	
yycrank+0,	yysvec+138,	yyvstop+285,
yycrank+134,	yysvec+34,	yyvstop+287,
yycrank+0,	yysvec+34,	yyvstop+289,
yycrank+143,	yysvec+34,	yyvstop+292,
yycrank+118,	yysvec+34,	yyvstop+294,
yycrank+100,	yysvec+34,	yyvstop+296,
yycrank+117,	yysvec+34,	yyvstop+298,
yycrank+104,	yysvec+34,	yyvstop+300,
yycrank+0,	yysvec+34,	yyvstop+302,
yycrank+0,	yysvec+34,	yyvstop+305,
yycrank+121,	yysvec+34,	yyvstop+308,
yycrank+129,	yysvec+34,	yyvstop+310,
yycrank+0,	yysvec+34,	yyvstop+312,
yycrank+130,	yysvec+34,	yyvstop+315,
yycrank+0,	yysvec+34,	yyvstop+317,
yycrank+118,	yysvec+34,	yyvstop+320,
yycrank+129,	yysvec+34,	yyvstop+322,
yycrank+136,	yysvec+34,	yyvstop+324,
yycrank+137,	yysvec+34,	yyvstop+326,
yycrank+128,	yysvec+34,	yyvstop+328,
yycrank+130,	yysvec+34,	yyvstop+330,
yycrank+138,	yysvec+34,	yyvstop+332,
yycrank+398,	0,		0,	
yycrank+169,	yysvec+34,	yyvstop+334,
yycrank+180,	yysvec+34,	yyvstop+336,
yycrank+143,	yysvec+34,	yyvstop+338,
yycrank+157,	yysvec+34,	yyvstop+340,
yycrank+0,	yysvec+34,	yyvstop+342,
yycrank+0,	yysvec+34,	yyvstop+345,
yycrank+159,	yysvec+34,	yyvstop+348,
yycrank+145,	yysvec+34,	yyvstop+350,
yycrank+150,	yysvec+34,	yyvstop+352,
yycrank+0,	yysvec+34,	yyvstop+354,
yycrank+152,	yysvec+34,	yyvstop+357,
yycrank+153,	yysvec+34,	yyvstop+359,
yycrank+163,	yysvec+34,	yyvstop+361,
yycrank+0,	yysvec+34,	yyvstop+363,
yycrank+158,	yysvec+34,	yyvstop+366,
yycrank+174,	yysvec+34,	yyvstop+368,
yycrank+0,	0,		yyvstop+370,
yycrank+0,	yysvec+34,	yyvstop+372,
yycrank+222,	yysvec+34,	yyvstop+375,
yycrank+0,	yysvec+34,	yyvstop+377,
yycrank+192,	yysvec+34,	yyvstop+380,
yycrank+193,	yysvec+34,	yyvstop+382,
yycrank+0,	yysvec+34,	yyvstop+384,
yycrank+200,	yysvec+34,	yyvstop+387,
yycrank+203,	yysvec+34,	yyvstop+389,
yycrank+0,	yysvec+34,	yyvstop+392,
yycrank+190,	yysvec+34,	yyvstop+395,
yycrank+193,	yysvec+34,	yyvstop+397,
yycrank+0,	yysvec+34,	yyvstop+399,
yycrank+240,	yysvec+34,	yyvstop+402,
yycrank+194,	yysvec+34,	yyvstop+404,
yycrank+211,	yysvec+34,	yyvstop+406,
yycrank+0,	yysvec+34,	yyvstop+408,
yycrank+0,	yysvec+34,	yyvstop+411,
yycrank+211,	yysvec+34,	yyvstop+414,
yycrank+0,	yysvec+34,	yyvstop+416,
yycrank+0,	yysvec+34,	yyvstop+419,
yycrank+213,	yysvec+34,	yyvstop+422,
yycrank+0,	yysvec+34,	yyvstop+424,
yycrank+0,	yysvec+34,	yyvstop+427,
yycrank+0,	yysvec+34,	yyvstop+430,
0,	0,	0};
struct yywork *yytop = yycrank+455;
struct yysvf *yybgin = yysvec+1;
char yymatch[] ={
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,'A' ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,01  ,
0};
char yyextra[] ={
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/*	ncform	4.1	83/08/11	*/

int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
char yysbuf[YYLMAX];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank){		/* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
			*yylastch++ = yych = input();
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
yyback(p, m)
	int *p;
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	}
yyoutput(c)
  int c; {
	output(c);
	}
yyunput(c)
   int c; {
	unput(c);
	}
