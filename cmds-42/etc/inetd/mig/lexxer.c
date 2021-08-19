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
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#if	NeXT
#include <sys/message.h>
#else	NeXT
#include <mach/message.h>
#endif	NeXT
#include "string.h"
#include "type.h"
#include "statement.h"
#include "global.h"
#include "parser.h"
#include "lexxer.h"

#ifdef	LDEBUG
#define RETURN(sym)							\
{									\
    printf("yylex: returning 'sym' (%d)\n", (sym));			\
    return (sym);							\
}
#else	LDEBUG
#define RETURN(sym)	return (sym)
#endif	LDEBUG

#define	PRIMRETURN(inN, inS, outN, outS, tsize)				\
{									\
    yylval.symtype.innumber = (inN);					\
    yylval.symtype.instr = (inS);					\
    yylval.symtype.outnumber = (outN);					\
    yylval.symtype.outstr = (outS);					\
    yylval.symtype.size = (tsize);					\
    RETURN(sySymbolicType);						\
}

#if	__GNU__
#define	TRETURN(type, tsize)	PRIMRETURN(type,#type,type,#type,tsize)
#define TPRETURN(intype, outtype, tsize)	\
			PRIMRETURN(intype,#intype,outtype,#outtype,tsize)
#else	__GNU__
#define	TRETURN(type, tsize)	PRIMRETURN(type,"type",type,"type",tsize)
#define TPRETURN(intype, outtype, tsize)	\
			PRIMRETURN(intype,"intype",outtype,"outtype",tsize)
#endif	__GNU__

#define	FRETURN(val)							\
{									\
    yylval.flag = (val);						\
    RETURN(syIPCFlag);							\
}

static struct yysvf *oldYYBegin;

static void doSharp(); /* process body of # directives */
# define Normal 2
# define String 4
# define FileName 6
# define QString 8
# define SkipToEOL 10
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
	RETURN(syRoutine);
break;
case 2:
RETURN(syFunction);
break;
case 3:
RETURN(syProcedure);
break;
case 4:
RETURN(sySimpleProcedure);
break;
case 5:
RETURN(sySimpleRoutine);
break;
case 6:
RETURN(syCamelotRoutine);
break;
case 7:
RETURN(sySubsystem);
break;
case 8:
	RETURN(syMsgType);
break;
case 9:
RETURN(syWaitTime);
break;
case 10:
RETURN(syNoWaitTime);
break;
case 11:
			RETURN(syIn);
break;
case 12:
			RETURN(syOut);
break;
case 13:
		RETURN(syInOut);
break;
case 14:
RETURN(syRequestPort);
break;
case 15:
RETURN(syReplyPort);
break;
case 16:
		RETURN(syArray);
break;
case 17:
			RETURN(syOf);
break;
case 18:
		RETURN(syErrorProc);
break;
case 19:
RETURN(syServerPrefix);
break;
case 20:
RETURN(syUserPrefix);
break;
case 21:
		RETURN(syRCSId);
break;
case 22:
	RETURN(syImport);
break;
case 23:
	RETURN(syUImport);
break;
case 24:
	RETURN(sySImport);
break;
case 25:
		RETURN(syType);
break;
case 26:
	RETURN(syKernel);
break;
case 27:
	RETURN(syCamelot);
break;
case 28:
		RETURN(sySkip);
break;
case 29:
	RETURN(syStruct);
break;
case 30:
	RETURN(syInTran);
break;
case 31:
	RETURN(syOutTran);
break;
case 32:
RETURN(syDestructor);
break;
case 33:
			RETURN(syCType);
break;
case 34:
	RETURN(syCUserType);
break;
case 35:
RETURN(syCServerType);
break;
case 36:
	FRETURN(flLong);
break;
case 37:
FRETURN(flNotLong);
break;
case 38:
	FRETURN(flDealloc);
break;
case 39:
FRETURN(flNotDealloc);
break;
case 40:
TRETURN(MSG_TYPE_POLYMORPHIC,32);
break;
case 41:
	TRETURN(MSG_TYPE_UNSTRUCTURED,0);
break;
case 42:
		TRETURN(MSG_TYPE_BIT,1);
break;
case 43:
	TRETURN(MSG_TYPE_BOOLEAN,32);
break;
case 44:
	TRETURN(MSG_TYPE_INTEGER_16,16);
break;
case 45:
	TRETURN(MSG_TYPE_INTEGER_32,32);
break;
case 46:
	TRETURN(MSG_TYPE_PORT_ALL,32);
break;
case 47:
		TRETURN(MSG_TYPE_PORT,32);
break;
case 48:
		TRETURN(MSG_TYPE_CHAR,8);
break;
case 49:
		TRETURN(MSG_TYPE_BYTE,8);
break;
case 50:
	TRETURN(MSG_TYPE_INTEGER_8,8);
break;
case 51:
		TRETURN(MSG_TYPE_REAL,0);
break;
case 52:
	TRETURN(MSG_TYPE_STRING,0);
break;
case 53:
	TRETURN(MSG_TYPE_STRING_C,0);
break;
case 54:
	TRETURN(MSG_TYPE_PORT_NAME,0);
break;
case 55:
TRETURN(MSG_TYPE_INTERNAL_MEMORY,8);
break;
case 56:
	TRETURN(MSG_TYPE_POLYMORPHIC,0);
break;
case 57:
	RETURN(syColon);
break;
case 58:
	RETURN(sySemi);
break;
case 59:
	RETURN(syComma);
break;
case 60:
	RETURN(syPlus);
break;
case 61:
	RETURN(syMinus);
break;
case 62:
	RETURN(syStar);
break;
case 63:
	RETURN(syDiv);
break;
case 64:
	RETURN(syLParen);
break;
case 65:
	RETURN(syRParen);
break;
case 66:
	RETURN(syEqual);
break;
case 67:
	RETURN(syCaret);
break;
case 68:
	RETURN(syTilde);
break;
case 69:
	RETURN(syLAngle);
break;
case 70:
	RETURN(syRAngle);
break;
case 71:
	RETURN(syLBrack);
break;
case 72:
	RETURN(syRBrack);
break;
case 73:
	RETURN(syBar);
break;
case 74:
	{ yylval.identifier = strmake(yytext);
			  RETURN(syIdentifier); }
break;
case 75:
{ yylval.number = atoi(yytext); RETURN(syNumber); }
break;
case 76:
{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syString); }
break;
case 77:
{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syFileName); }
break;
case 78:
{ yylval.string = strmake(yytext);
			  BEGIN Normal; RETURN(syQString); }
break;
case 79:
{ doSharp(yytext+1);
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }
break;
case 80:
			{ doSharp(yytext+1);
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }
break;
case 81:
				{ yyerror("illegal # directive");
					  oldYYBegin = yybgin;
					  BEGIN SkipToEOL; }
break;
case 82:
	yybgin = oldYYBegin;
break;
case 83:
	;
break;
case 84:
		;
break;
case 85:
		{ BEGIN Normal; RETURN(syError); }
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */

extern void
LookNormal()
{
    BEGIN Normal;
}

extern void
LookString()
{
    BEGIN String;
}

extern void
LookQString()
{
    BEGIN QString;
}

extern void
LookFileName()
{
    BEGIN FileName;
}

static void
doSharp(body)
    char *body;
{
    register char *startName;

    yylineno = atoi(body);
    startName = index(body, '"');
    if (startName != NULL)
    {
	char	*c;

	c = rindex(body, '"');
	*c = '\0';
	strfree(yyinname);
	yyinname = strmake(startName+1);
    }
}
int yyvstop[] ={
0,

85,
0,

84,
85,
0,

84,
0,

81,
85,
0,

64,
85,
0,

65,
85,
0,

62,
85,
0,

60,
85,
0,

59,
85,
0,

61,
85,
0,

63,
85,
0,

75,
85,
0,

57,
85,
0,

58,
85,
0,

69,
85,
0,

66,
85,
0,

70,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

74,
85,
0,

71,
85,
0,

72,
85,
0,

67,
85,
0,

74,
85,
0,

73,
85,
0,

68,
85,
0,

76,
85,
0,

85,
0,

85,
0,

85,
0,

83,
85,
0,

83,
84,
85,
0,

82,
84,
0,

81,
83,
85,
0,

80,
0,

75,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

11,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

17,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

76,
0,

77,
0,

78,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

12,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

79,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

28,
74,
0,

74,
0,

74,
0,

25,
74,
0,

74,
0,

74,
0,

74,
0,

16,
74,
0,

74,
0,

74,
0,

33,
74,
0,

74,
0,

74,
0,

74,
0,

18,
74,
0,

74,
0,

74,
0,

13,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

21,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

22,
74,
0,

30,
74,
0,

36,
74,
0,

74,
0,

26,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

29,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

27,
74,
0,

74,
0,

74,
0,

38,
74,
0,

74,
0,

74,
0,

74,
0,

8,
74,
0,

74,
0,

74,
0,

74,
0,

31,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

1,
74,
0,

74,
0,

74,
0,

74,
0,

24,
74,
0,

74,
0,

23,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

2,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

9,
74,
0,

74,
0,

74,
0,

34,
74,
0,

74,
0,

37,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

3,
74,
0,

15,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

7,
74,
0,

74,
0,

74,
0,

74,
0,

32,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

39,
74,
0,

10,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

20,
74,
0,

74,
0,

35,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

40,
74,
0,

14,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

42,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

19,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

49,
74,
0,

48,
74,
0,

74,
0,

74,
0,

47,
74,
0,

51,
74,
0,

74,
0,

74,
0,

74,
0,

5,
74,
0,

6,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

52,
74,
0,

74,
0,

4,
74,
0,

43,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

46,
74,
0,

74,
0,

53,
74,
0,

74,
0,

74,
0,

74,
0,

50,
74,
0,

74,
0,

74,
0,

54,
74,
0,

74,
0,

44,
74,
0,

45,
74,
0,

74,
0,

74,
0,

74,
0,

74,
0,

56,
74,
0,

74,
0,

74,
0,

41,
74,
0,

74,
0,

74,
0,

55,
74,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] ={
0,0,	0,0,	1,13,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,14,	1,15,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	1,13,	
2,16,	1,13,	7,54,	8,54,	
8,16,	9,56,	10,56,	10,16,	
12,60,	0,0,	0,0,	0,0,	
0,0,	1,13,	24,64,	24,64,	
24,64,	24,64,	24,64,	24,64,	
24,64,	24,64,	24,64,	24,64,	
0,0,	0,0,	0,0,	1,13,	
7,55,	8,55,	1,13,	1,13,	
1,13,	1,13,	1,13,	1,13,	
1,13,	1,13,	1,13,	1,13,	
1,13,	1,13,	1,13,	1,13,	
1,13,	1,13,	1,13,	1,13,	
1,13,	1,13,	1,13,	1,13,	
1,13,	1,13,	1,13,	3,13,	
98,0,	31,65,	63,104,	32,67,	
33,71,	34,72,	100,0,	3,14,	
3,15,	101,0,	0,0,	0,0,	
35,65,	0,0,	0,0,	35,73,	
37,77,	33,65,	38,65,	38,78,	
32,65,	32,68,	32,69,	32,70,	
0,0,	63,104,	0,0,	63,105,	
0,0,	37,65,	65,65,	0,0,	
3,13,	31,65,	3,13,	32,67,	
33,71,	34,72,	3,17,	3,18,	
3,19,	3,20,	3,21,	3,22,	
35,65,	3,23,	3,24,	35,73,	
37,77,	33,65,	38,65,	38,79,	
32,65,	32,68,	32,69,	32,70,	
3,25,	3,26,	3,27,	3,28,	
3,29,	37,65,	65,65,	3,30,	
3,31,	3,32,	3,33,	3,34,	
3,35,	3,31,	3,31,	3,36,	
3,31,	3,37,	3,31,	3,38,	
3,39,	3,40,	3,41,	3,31,	
3,42,	3,43,	3,44,	3,45,	
3,31,	3,46,	3,31,	3,31,	
4,16,	3,47,	0,0,	3,48,	
3,49,	4,17,	4,18,	4,19,	
4,20,	4,21,	4,22,	39,80,	
4,23,	66,106,	39,65,	36,74,	
36,75,	40,81,	46,96,	3,50,	
36,65,	36,76,	0,0,	4,25,	
4,26,	4,27,	4,28,	0,0,	
0,0,	40,65,	41,83,	0,0,	
40,82,	41,84,	3,51,	46,65,	
3,52,	16,61,	50,65,	50,79,	
72,113,	68,108,	0,0,	39,80,	
0,0,	66,106,	39,65,	36,74,	
36,75,	40,81,	46,96,	0,0,	
36,65,	36,76,	68,65,	77,120,	
4,47,	0,0,	4,48,	4,49,	
16,62,	40,65,	41,83,	5,53,	
40,82,	41,84,	0,0,	46,65,	
0,0,	0,0,	50,65,	50,79,	
72,113,	68,108,	4,50,	5,53,	
16,63,	16,63,	16,63,	16,63,	
16,63,	16,63,	16,63,	16,63,	
16,63,	16,63,	68,65,	77,120,	
74,115,	4,51,	74,65,	4,52,	
5,53,	5,53,	5,53,	5,53,	
5,53,	5,53,	5,53,	5,53,	
5,53,	5,53,	5,53,	5,53,	
5,53,	5,53,	5,53,	5,53,	
5,53,	5,53,	5,53,	5,53,	
5,53,	5,53,	5,53,	5,53,	
5,53,	6,16,	6,53,	61,61,	
74,115,	0,0,	74,65,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	6,53,	0,0,	
0,0,	0,0,	0,0,	61,103,	
61,103,	61,103,	61,103,	61,103,	
61,103,	61,103,	61,103,	61,103,	
61,103,	81,65,	0,0,	6,53,	
6,53,	6,53,	6,53,	6,53,	
6,53,	6,53,	6,53,	6,53,	
6,53,	6,53,	6,53,	6,53,	
6,53,	6,53,	6,53,	6,53,	
6,53,	6,53,	6,53,	6,53,	
6,53,	6,53,	6,53,	6,53,	
11,57,	0,0,	42,85,	0,0,	
42,86,	81,65,	43,88,	88,132,	
11,58,	11,59,	43,89,	44,65,	
43,90,	69,65,	42,87,	0,0,	
45,94,	42,65,	44,93,	43,65,	
69,109,	43,91,	43,92,	67,107,	
84,127,	45,65,	45,95,	84,65,	
67,65,	0,0,	70,65,	70,110,	
0,0,	11,57,	42,85,	11,57,	
42,86,	0,0,	43,88,	88,132,	
0,0,	0,0,	43,89,	44,65,	
43,90,	69,65,	42,87,	11,57,	
45,94,	42,65,	44,93,	43,65,	
69,109,	43,91,	43,92,	67,107,	
84,127,	45,65,	45,95,	84,65,	
67,65,	11,57,	70,65,	70,110,	
11,57,	11,57,	11,57,	11,57,	
11,57,	11,57,	11,57,	11,57,	
11,57,	11,57,	11,57,	11,57,	
11,57,	11,57,	11,57,	11,57,	
11,57,	11,57,	11,57,	11,57,	
11,57,	11,57,	11,57,	11,57,	
11,57,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	0,0,	
82,65,	73,114,	82,125,	91,135,	
0,0,	73,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,66,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
82,65,	73,114,	82,125,	91,135,	
30,65,	73,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,66,	
30,65,	30,65,	30,65,	30,65,	
30,65,	30,65,	30,65,	30,65,	
53,97,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	0,0,	75,116,	
85,65,	85,128,	75,65,	0,0,	
75,117,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	75,116,	
85,65,	85,128,	75,65,	53,97,	
75,117,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	53,97,	
53,97,	53,97,	53,97,	54,98,	
0,0,	71,111,	0,0,	76,118,	
78,121,	76,119,	79,122,	54,98,	
54,0,	76,65,	93,137,	80,65,	
93,65,	80,123,	0,0,	78,65,	
80,124,	79,65,	71,65,	71,112,	
83,126,	86,129,	86,130,	86,65,	
87,65,	89,133,	83,65,	87,131,	
0,0,	108,144,	89,65,	0,0,	
54,99,	71,111,	54,98,	76,118,	
78,122,	76,119,	79,122,	0,0,	
0,0,	76,65,	93,137,	80,65,	
93,65,	80,123,	54,98,	78,65,	
80,124,	79,65,	71,65,	71,112,	
83,126,	86,129,	86,130,	86,65,	
87,65,	89,133,	83,65,	87,131,	
54,98,	108,144,	89,65,	54,98,	
54,98,	54,98,	54,98,	54,98,	
54,98,	54,98,	54,98,	54,98,	
54,98,	54,98,	54,98,	54,98,	
54,98,	54,98,	54,98,	54,98,	
54,98,	54,98,	54,98,	54,98,	
54,98,	54,98,	54,98,	54,98,	
55,100,	104,104,	90,134,	0,0,	
92,136,	94,138,	95,139,	96,140,	
55,100,	55,0,	94,65,	90,65,	
107,143,	106,142,	110,146,	0,0,	
96,65,	117,153,	137,173,	95,65,	
92,65,	109,145,	111,147,	109,65,	
104,104,	107,65,	104,105,	110,65,	
111,65,	0,0,	106,65,	137,65,	
0,0,	55,100,	90,134,	55,100,	
92,136,	94,138,	95,139,	96,140,	
0,0,	0,0,	94,65,	90,65,	
107,143,	106,142,	110,146,	55,100,	
96,65,	117,153,	137,173,	95,65,	
92,65,	109,145,	111,147,	109,65,	
0,0,	107,65,	139,175,	110,65,	
111,65,	55,99,	106,65,	137,65,	
55,100,	55,100,	55,100,	55,100,	
55,100,	55,100,	55,100,	55,100,	
55,100,	55,100,	55,100,	55,100,	
55,100,	55,100,	55,100,	55,100,	
55,100,	55,100,	55,100,	55,100,	
55,100,	55,100,	55,100,	55,100,	
55,100,	56,101,	139,175,	0,0,	
0,0,	0,0,	112,65,	129,165,	
112,148,	56,101,	56,0,	113,149,	
103,104,	129,65,	113,65,	115,151,	
0,0,	114,150,	115,65,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	121,65,	0,0,	121,157,	
114,65,	0,0,	56,102,	103,104,	
56,101,	103,105,	112,65,	129,165,	
112,148,	0,0,	121,158,	113,149,	
0,0,	129,65,	113,65,	115,151,	
56,101,	114,150,	115,65,	103,103,	
103,103,	103,103,	103,103,	103,103,	
103,103,	103,103,	103,103,	103,103,	
103,103,	121,65,	56,101,	121,157,	
114,65,	56,101,	56,101,	56,101,	
56,101,	56,101,	56,101,	56,101,	
56,101,	56,101,	56,101,	56,101,	
56,101,	56,101,	56,101,	56,101,	
56,101,	56,101,	56,101,	56,101,	
56,101,	56,101,	56,101,	56,101,	
56,101,	56,101,	105,105,	0,0,	
116,65,	0,0,	118,154,	116,152,	
123,159,	118,65,	105,105,	105,105,	
119,155,	124,160,	120,156,	119,65,	
146,181,	0,0,	120,65,	122,65,	
126,65,	122,157,	123,65,	125,65,	
128,164,	125,161,	130,65,	126,162,	
132,65,	130,166,	124,65,	0,0,	
132,168,	128,65,	0,0,	105,141,	
116,65,	105,105,	118,154,	116,152,	
123,159,	118,65,	0,0,	0,0,	
119,155,	124,160,	120,156,	119,65,	
146,181,	105,105,	120,65,	122,65,	
126,65,	122,157,	123,65,	125,65,	
128,164,	125,161,	130,65,	126,162,	
132,65,	130,166,	124,65,	105,105,	
132,168,	128,65,	105,105,	105,105,	
105,105,	105,105,	105,105,	105,105,	
105,105,	105,105,	105,105,	105,105,	
105,105,	105,105,	105,105,	105,105,	
105,105,	105,105,	105,105,	105,105,	
105,105,	105,105,	105,105,	105,105,	
105,105,	105,105,	105,105,	127,163,	
131,65,	133,169,	131,167,	133,65,	
134,170,	135,65,	134,65,	0,0,	
135,171,	136,65,	136,172,	138,174,	
142,65,	138,65,	127,65,	140,65,	
143,178,	140,176,	148,183,	142,177,	
144,65,	147,182,	143,65,	149,184,	
144,179,	151,186,	145,180,	147,65,	
150,65,	0,0,	150,185,	127,163,	
131,65,	133,169,	131,167,	133,65,	
134,170,	135,65,	134,65,	145,65,	
135,171,	136,65,	136,172,	138,174,	
142,65,	138,65,	127,65,	140,65,	
143,178,	140,176,	148,183,	142,177,	
144,65,	147,182,	143,65,	149,184,	
144,179,	151,186,	145,180,	147,65,	
150,65,	152,65,	150,185,	152,187,	
153,188,	154,189,	155,65,	156,191,	
155,190,	154,65,	157,65,	145,65,	
158,65,	159,194,	158,193,	160,195,	
161,196,	157,192,	162,197,	163,198,	
156,65,	153,65,	0,0,	162,65,	
160,65,	0,0,	159,65,	165,65,	
0,0,	164,199,	0,0,	170,65,	
163,65,	152,65,	165,200,	152,187,	
153,188,	154,189,	155,65,	156,191,	
155,190,	154,65,	157,65,	164,65,	
158,65,	159,194,	0,0,	160,195,	
161,196,	157,192,	162,197,	163,198,	
156,65,	153,65,	166,201,	162,65,	
160,65,	168,203,	159,65,	165,65,	
167,202,	164,199,	169,204,	170,65,	
163,65,	169,205,	165,200,	166,65,	
169,65,	167,65,	168,65,	172,65,	
171,206,	173,65,	174,208,	164,65,	
177,65,	174,65,	172,207,	175,209,	
176,65,	175,65,	176,210,	179,212,	
180,65,	178,211,	166,201,	171,65,	
178,65,	168,203,	184,65,	181,65,	
167,202,	181,213,	169,204,	182,214,	
179,65,	169,205,	182,65,	166,65,	
169,65,	167,65,	168,65,	172,65,	
171,206,	173,65,	174,208,	187,65,	
177,65,	174,65,	172,207,	175,209,	
176,65,	175,65,	176,210,	179,212,	
180,65,	178,211,	183,65,	171,65,	
178,65,	183,215,	184,65,	181,65,	
185,216,	181,213,	189,219,	182,214,	
179,65,	186,65,	182,65,	186,217,	
188,218,	185,65,	190,220,	191,221,	
188,65,	189,65,	193,65,	187,65,	
190,65,	191,65,	192,222,	194,224,	
192,65,	193,223,	195,65,	199,65,	
195,225,	197,227,	183,65,	0,0,	
197,65,	183,215,	196,226,	200,229,	
185,216,	200,65,	189,219,	0,0,	
194,65,	186,65,	0,0,	186,217,	
188,218,	185,65,	190,220,	191,221,	
188,65,	189,65,	193,65,	196,65,	
190,65,	191,65,	192,222,	194,224,	
192,65,	198,228,	195,65,	199,65,	
195,225,	197,227,	201,65,	201,230,	
197,65,	203,232,	196,226,	200,229,	
204,233,	200,65,	202,231,	198,65,	
194,65,	205,234,	202,65,	206,65,	
208,237,	206,235,	207,65,	207,236,	
209,238,	204,65,	210,239,	196,65,	
211,65,	212,241,	211,240,	0,0,	
217,65,	198,228,	216,245,	210,65,	
213,65,	216,65,	201,65,	201,230,	
218,65,	203,232,	0,0,	213,242,	
204,233,	219,65,	202,231,	198,65,	
214,243,	205,234,	202,65,	206,65,	
208,237,	206,235,	207,65,	207,236,	
209,238,	204,65,	210,239,	215,244,	
211,65,	212,241,	211,240,	214,65,	
217,65,	221,65,	216,245,	210,65,	
213,65,	216,65,	220,246,	222,247,	
218,65,	220,65,	215,65,	213,242,	
223,248,	219,65,	223,65,	224,249,	
214,243,	225,65,	226,251,	225,250,	
222,65,	224,65,	226,65,	227,252,	
231,256,	228,65,	229,254,	215,244,	
228,253,	229,65,	230,65,	214,65,	
230,255,	221,65,	232,257,	235,65,	
232,65,	231,65,	220,246,	222,247,	
0,0,	220,65,	215,65,	233,258,	
0,0,	233,259,	223,65,	224,249,	
240,265,	225,65,	226,251,	225,250,	
222,65,	224,65,	226,65,	227,252,	
231,256,	228,65,	229,254,	243,65,	
228,253,	229,65,	230,65,	234,65,	
230,255,	234,260,	232,257,	235,65,	
232,65,	231,65,	236,65,	237,65,	
236,261,	237,262,	238,263,	233,258,	
239,264,	233,259,	247,65,	248,271,	
240,265,	239,65,	241,65,	242,267,	
241,266,	242,65,	244,65,	238,65,	
244,268,	245,269,	246,270,	243,65,	
248,65,	245,65,	246,65,	234,65,	
251,65,	234,260,	253,275,	254,276,	
250,273,	249,272,	236,65,	237,65,	
236,261,	237,262,	238,263,	249,65,	
239,264,	250,65,	247,65,	256,65,	
257,278,	239,65,	241,65,	242,267,	
241,266,	242,65,	244,65,	238,65,	
244,268,	245,269,	246,270,	258,279,	
248,65,	245,65,	246,65,	252,274,	
251,65,	252,65,	253,275,	254,276,	
250,273,	249,272,	255,277,	259,280,	
255,65,	260,65,	259,65,	249,65,	
261,281,	250,65,	262,65,	256,65,	
257,278,	263,282,	265,284,	264,283,	
269,65,	265,65,	267,286,	266,65,	
0,0,	261,65,	0,0,	258,279,	
0,0,	263,65,	266,285,	252,274,	
264,65,	252,65,	268,287,	267,65,	
270,288,	268,65,	255,277,	259,280,	
255,65,	260,65,	259,65,	271,65,	
261,281,	272,290,	262,65,	270,65,	
272,65,	263,282,	265,284,	264,283,	
269,65,	265,65,	267,286,	266,65,	
271,289,	261,65,	273,291,	275,293,	
274,292,	263,65,	266,285,	273,65,	
264,65,	0,0,	268,287,	267,65,	
270,288,	268,65,	274,65,	276,65,	
275,65,	276,294,	278,296,	271,65,	
283,65,	272,290,	277,295,	270,65,	
272,65,	277,65,	279,297,	280,65,	
281,299,	279,65,	280,298,	278,65,	
282,300,	281,65,	273,291,	275,293,	
274,292,	284,65,	286,65,	273,65,	
284,301,	282,65,	285,302,	287,303,	
285,65,	288,65,	274,65,	276,65,	
275,65,	276,294,	278,296,	290,311,	
283,65,	0,0,	277,295,	291,312,	
293,65,	277,65,	279,297,	280,65,	
281,299,	279,65,	280,298,	278,65,	
282,300,	281,65,	290,65,	294,65,	
291,65,	284,65,	286,65,	292,313,	
284,301,	282,65,	285,302,	287,303,	
285,65,	288,65,	289,304,	289,305,	
292,65,	295,314,	299,65,	290,311,	
296,315,	289,306,	297,316,	291,312,	
293,65,	298,65,	303,65,	298,317,	
289,307,	0,0,	289,308,	289,309,	
296,65,	289,310,	290,65,	294,65,	
291,65,	297,65,	300,65,	292,313,	
301,65,	305,324,	301,319,	302,320,	
300,318,	0,0,	0,0,	0,0,	
292,65,	295,314,	299,65,	305,65,	
296,315,	308,327,	297,316,	0,0,	
302,65,	298,65,	303,65,	298,317,	
307,326,	306,325,	289,65,	307,65,	
296,65,	306,65,	308,65,	311,65,	
312,65,	297,65,	300,65,	304,321,	
301,65,	318,65,	301,319,	302,320,	
300,318,	304,322,	310,329,	309,65,	
304,65,	309,328,	310,65,	305,65,	
315,332,	317,334,	313,330,	304,323,	
302,65,	314,65,	316,333,	314,331,	
320,65,	315,65,	317,65,	307,65,	
0,0,	306,65,	308,65,	311,65,	
312,65,	313,65,	324,339,	316,65,	
319,335,	318,65,	321,65,	322,337,	
321,336,	326,341,	322,65,	309,65,	
304,65,	319,65,	310,65,	326,342,	
315,332,	317,334,	313,330,	324,65,	
327,343,	314,65,	316,333,	314,331,	
320,65,	315,65,	317,65,	323,65,	
325,65,	323,338,	325,340,	328,344,	
330,65,	313,65,	331,65,	316,65,	
319,335,	327,65,	321,65,	329,65,	
329,345,	333,347,	322,65,	332,65,	
334,348,	319,65,	335,349,	326,65,	
334,65,	332,346,	335,65,	324,65,	
336,65,	339,352,	338,351,	333,65,	
337,350,	340,353,	341,65,	323,65,	
325,65,	346,65,	337,65,	328,65,	
330,65,	341,354,	331,65,	338,65,	
351,65,	327,65,	340,65,	329,65,	
342,65,	333,347,	342,355,	332,65,	
334,348,	343,356,	335,349,	344,357,	
334,65,	332,346,	335,65,	343,65,	
336,65,	339,65,	352,65,	333,65,	
344,65,	345,65,	341,65,	345,358,	
347,65,	346,65,	337,65,	347,359,	
348,360,	349,361,	350,362,	338,65,	
351,65,	353,363,	340,65,	354,365,	
342,65,	356,65,	355,65,	358,368,	
354,65,	348,65,	349,65,	350,65,	
353,364,	357,367,	359,369,	343,65,	
360,65,	357,65,	352,65,	355,366,	
344,65,	345,65,	361,65,	363,371,	
347,65,	0,0,	0,0,	347,359,	
348,360,	349,361,	362,370,	364,372,	
0,0,	371,380,	373,382,	364,65,	
363,65,	356,65,	355,65,	358,65,	
354,65,	348,65,	349,65,	350,65,	
353,65,	366,374,	359,369,	362,65,	
360,65,	357,65,	365,373,	369,378,	
367,376,	365,65,	361,65,	372,381,	
368,65,	370,379,	366,375,	368,377,	
375,384,	370,65,	366,65,	367,65,	
369,65,	371,65,	373,65,	364,65,	
363,65,	374,383,	378,65,	376,65,	
372,65,	379,65,	377,386,	374,65,	
381,388,	375,65,	380,65,	362,65,	
383,390,	0,0,	381,65,	369,378,	
376,385,	365,65,	383,65,	390,65,	
368,65,	377,65,	382,389,	380,387,	
382,65,	370,65,	366,65,	367,65,	
369,65,	385,392,	386,65,	392,65,	
386,393,	384,391,	378,65,	376,65,	
372,65,	379,65,	384,65,	374,65,	
393,65,	375,65,	380,65,	393,400,	
385,65,	387,394,	381,65,	387,395,	
388,65,	389,398,	383,65,	390,65,	
387,396,	377,65,	391,399,	394,401,	
382,65,	395,402,	396,65,	389,65,	
397,403,	388,397,	386,65,	392,65,	
398,404,	397,65,	399,65,	391,65,	
400,405,	401,65,	384,65,	402,65,	
393,65,	398,65,	403,406,	405,408,	
385,65,	404,407,	387,65,	406,409,	
388,65,	407,65,	408,410,	394,65,	
406,65,	410,65,	411,412,	403,65,	
405,65,	395,65,	396,65,	389,65,	
404,65,	413,65,	0,0,	412,65,	
408,65,	397,65,	399,65,	391,65,	
400,65,	401,65,	412,413,	402,65,	
409,411,	398,65,	0,0,	409,65,	
0,0,	0,0,	387,65,	0,0,	
0,0,	407,65,	0,0,	394,65,	
406,65,	410,65,	411,65,	403,65,	
405,65,	395,65,	0,0,	0,0,	
404,65,	413,65,	0,0,	412,65,	
408,65,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	409,65,	
0,0};
struct yysvf yysvec[] ={
0,	0,	0,
yycrank+-1,	0,		0,	
yycrank+-1,	yysvec+1,	0,	
yycrank+-90,	0,		0,	
yycrank+-145,	yysvec+3,	0,	
yycrank+-207,	yysvec+1,	0,	
yycrank+-262,	yysvec+1,	0,	
yycrank+-4,	yysvec+1,	0,	
yycrank+-5,	yysvec+1,	0,	
yycrank+-7,	yysvec+1,	0,	
yycrank+-8,	yysvec+1,	0,	
yycrank+-351,	0,		0,	
yycrank+-9,	yysvec+11,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+3,
yycrank+0,	0,		yyvstop+6,
yycrank+208,	0,		yyvstop+8,
yycrank+0,	0,		yyvstop+11,
yycrank+0,	0,		yyvstop+14,
yycrank+0,	0,		yyvstop+17,
yycrank+0,	0,		yyvstop+20,
yycrank+0,	0,		yyvstop+23,
yycrank+0,	0,		yyvstop+26,
yycrank+0,	0,		yyvstop+29,
yycrank+2,	0,		yyvstop+32,
yycrank+0,	0,		yyvstop+35,
yycrank+0,	0,		yyvstop+38,
yycrank+0,	0,		yyvstop+41,
yycrank+0,	0,		yyvstop+44,
yycrank+0,	0,		yyvstop+47,
yycrank+393,	0,		yyvstop+50,
yycrank+11,	yysvec+30,	yyvstop+53,
yycrank+30,	yysvec+30,	yyvstop+56,
yycrank+27,	yysvec+30,	yyvstop+59,
yycrank+15,	yysvec+30,	yyvstop+62,
yycrank+22,	yysvec+30,	yyvstop+65,
yycrank+118,	yysvec+30,	yyvstop+68,
yycrank+39,	yysvec+30,	yyvstop+71,
yycrank+28,	yysvec+30,	yyvstop+74,
yycrank+112,	yysvec+30,	yyvstop+77,
yycrank+127,	yysvec+30,	yyvstop+80,
yycrank+131,	yysvec+30,	yyvstop+83,
yycrank+287,	yysvec+30,	yyvstop+86,
yycrank+289,	yysvec+30,	yyvstop+89,
yycrank+281,	yysvec+30,	yyvstop+92,
yycrank+295,	yysvec+30,	yyvstop+95,
yycrank+133,	yysvec+30,	yyvstop+98,
yycrank+0,	0,		yyvstop+101,
yycrank+0,	0,		yyvstop+104,
yycrank+0,	0,		yyvstop+107,
yycrank+136,	yysvec+30,	yyvstop+110,
yycrank+0,	0,		yyvstop+113,
yycrank+0,	0,		yyvstop+116,
yycrank+480,	0,		yyvstop+119,
yycrank+-602,	0,		yyvstop+122,
yycrank+-691,	0,		yyvstop+124,
yycrank+-780,	0,		yyvstop+126,
yycrank+0,	0,		yyvstop+128,
yycrank+0,	0,		yyvstop+131,
yycrank+0,	0,		yyvstop+135,
yycrank+0,	yysvec+16,	yyvstop+138,
yycrank+267,	yysvec+16,	0,	
yycrank+0,	yysvec+16,	0,	
yycrank+85,	yysvec+16,	yyvstop+142,
yycrank+0,	yysvec+24,	yyvstop+144,
yycrank+40,	yysvec+30,	yyvstop+146,
yycrank+111,	yysvec+30,	yyvstop+148,
yycrank+298,	yysvec+30,	yyvstop+150,
yycrank+152,	yysvec+30,	yyvstop+152,
yycrank+283,	yysvec+30,	yyvstop+154,
yycrank+300,	yysvec+30,	yyvstop+156,
yycrank+540,	yysvec+30,	yyvstop+158,
yycrank+138,	yysvec+30,	yyvstop+160,
yycrank+375,	yysvec+30,	yyvstop+162,
yycrank+188,	yysvec+30,	yyvstop+164,
yycrank+460,	yysvec+30,	yyvstop+166,
yycrank+531,	yysvec+30,	yyvstop+169,
yycrank+153,	yysvec+30,	yyvstop+171,
yycrank+537,	yysvec+30,	yyvstop+173,
yycrank+539,	yysvec+30,	yyvstop+175,
yycrank+533,	yysvec+30,	yyvstop+177,
yycrank+243,	yysvec+30,	yyvstop+179,
yycrank+370,	yysvec+30,	yyvstop+182,
yycrank+548,	yysvec+30,	yyvstop+184,
yycrank+297,	yysvec+30,	yyvstop+186,
yycrank+458,	yysvec+30,	yyvstop+188,
yycrank+545,	yysvec+30,	yyvstop+190,
yycrank+546,	yysvec+30,	yyvstop+192,
yycrank+277,	yysvec+30,	yyvstop+194,
yycrank+552,	yysvec+30,	yyvstop+196,
yycrank+621,	yysvec+30,	yyvstop+198,
yycrank+373,	yysvec+30,	yyvstop+200,
yycrank+630,	yysvec+30,	yyvstop+202,
yycrank+534,	yysvec+30,	yyvstop+204,
yycrank+620,	yysvec+30,	yyvstop+206,
yycrank+629,	yysvec+30,	yyvstop+208,
yycrank+626,	yysvec+30,	yyvstop+210,
yycrank+0,	yysvec+53,	yyvstop+212,
yycrank+-82,	yysvec+54,	0,	
yycrank+0,	0,		yyvstop+214,
yycrank+-88,	yysvec+55,	0,	
yycrank+-91,	yysvec+56,	0,	
yycrank+0,	0,		yyvstop+216,
yycrank+783,	0,		0,	
yycrank+684,	0,		0,	
yycrank+-869,	0,		0,	
yycrank+640,	yysvec+30,	yyvstop+218,
yycrank+635,	yysvec+30,	yyvstop+220,
yycrank+551,	yysvec+30,	yyvstop+222,
yycrank+633,	yysvec+30,	yyvstop+224,
yycrank+637,	yysvec+30,	yyvstop+226,
yycrank+638,	yysvec+30,	yyvstop+228,
yycrank+704,	yysvec+30,	yyvstop+230,
yycrank+712,	yysvec+30,	yyvstop+232,
yycrank+730,	yysvec+30,	yyvstop+234,
yycrank+716,	yysvec+30,	yyvstop+236,
yycrank+790,	yysvec+30,	yyvstop+238,
yycrank+627,	yysvec+30,	yyvstop+240,
yycrank+795,	yysvec+30,	yyvstop+242,
yycrank+801,	yysvec+30,	yyvstop+244,
yycrank+804,	yysvec+30,	yyvstop+246,
yycrank+727,	yysvec+30,	yyvstop+248,
yycrank+805,	yysvec+30,	yyvstop+250,
yycrank+808,	yysvec+30,	yyvstop+252,
yycrank+816,	yysvec+30,	yyvstop+254,
yycrank+809,	yysvec+30,	yyvstop+256,
yycrank+806,	yysvec+30,	yyvstop+259,
yycrank+892,	yysvec+30,	yyvstop+261,
yycrank+819,	yysvec+30,	yyvstop+263,
yycrank+711,	yysvec+30,	yyvstop+265,
yycrank+812,	yysvec+30,	yyvstop+267,
yycrank+878,	yysvec+30,	yyvstop+269,
yycrank+814,	yysvec+30,	yyvstop+271,
yycrank+881,	yysvec+30,	yyvstop+273,
yycrank+884,	yysvec+30,	yyvstop+275,
yycrank+883,	yysvec+30,	yyvstop+277,
yycrank+887,	yysvec+30,	yyvstop+279,
yycrank+641,	yysvec+30,	yyvstop+281,
yycrank+891,	yysvec+30,	yyvstop+283,
yycrank+668,	yysvec+30,	yyvstop+285,
yycrank+893,	yysvec+30,	yyvstop+287,
yycrank+0,	0,		yyvstop+289,
yycrank+890,	yysvec+30,	yyvstop+291,
yycrank+900,	yysvec+30,	yyvstop+293,
yycrank+898,	yysvec+30,	yyvstop+295,
yycrank+917,	yysvec+30,	yyvstop+297,
yycrank+802,	yysvec+30,	yyvstop+299,
yycrank+905,	yysvec+30,	yyvstop+301,
yycrank+896,	yysvec+30,	yyvstop+303,
yycrank+901,	yysvec+30,	yyvstop+305,
yycrank+906,	yysvec+30,	yyvstop+307,
yycrank+903,	yysvec+30,	yyvstop+309,
yycrank+939,	yysvec+30,	yyvstop+311,
yycrank+959,	yysvec+30,	yyvstop+313,
yycrank+947,	yysvec+30,	yyvstop+315,
yycrank+944,	yysvec+30,	yyvstop+317,
yycrank+958,	yysvec+30,	yyvstop+319,
yycrank+948,	yysvec+30,	yyvstop+321,
yycrank+950,	yysvec+30,	yyvstop+323,
yycrank+964,	yysvec+30,	yyvstop+325,
yycrank+962,	yysvec+30,	yyvstop+327,
yycrank+954,	yysvec+30,	yyvstop+329,
yycrank+961,	yysvec+30,	yyvstop+331,
yycrank+970,	yysvec+30,	yyvstop+333,
yycrank+981,	yysvec+30,	yyvstop+335,
yycrank+965,	yysvec+30,	yyvstop+337,
yycrank+1005,	yysvec+30,	yyvstop+339,
yycrank+1007,	yysvec+30,	yyvstop+341,
yycrank+1008,	yysvec+30,	yyvstop+343,
yycrank+1006,	yysvec+30,	yyvstop+345,
yycrank+969,	yysvec+30,	yyvstop+347,
yycrank+1025,	yysvec+30,	yyvstop+350,
yycrank+1009,	yysvec+30,	yyvstop+352,
yycrank+1011,	yysvec+30,	yyvstop+354,
yycrank+1015,	yysvec+30,	yyvstop+357,
yycrank+1019,	yysvec+30,	yyvstop+359,
yycrank+1018,	yysvec+30,	yyvstop+361,
yycrank+1014,	yysvec+30,	yyvstop+363,
yycrank+1026,	yysvec+30,	yyvstop+366,
yycrank+1034,	yysvec+30,	yyvstop+368,
yycrank+1022,	yysvec+30,	yyvstop+370,
yycrank+1029,	yysvec+30,	yyvstop+373,
yycrank+1036,	yysvec+30,	yyvstop+375,
yycrank+1056,	yysvec+30,	yyvstop+377,
yycrank+1028,	yysvec+30,	yyvstop+379,
yycrank+1071,	yysvec+30,	yyvstop+382,
yycrank+1067,	yysvec+30,	yyvstop+384,
yycrank+1045,	yysvec+30,	yyvstop+386,
yycrank+1074,	yysvec+30,	yyvstop+389,
yycrank+1075,	yysvec+30,	yyvstop+391,
yycrank+1078,	yysvec+30,	yyvstop+393,
yycrank+1079,	yysvec+30,	yyvstop+395,
yycrank+1082,	yysvec+30,	yyvstop+397,
yycrank+1076,	yysvec+30,	yyvstop+399,
yycrank+1098,	yysvec+30,	yyvstop+401,
yycrank+1084,	yysvec+30,	yyvstop+403,
yycrank+1109,	yysvec+30,	yyvstop+405,
yycrank+1090,	yysvec+30,	yyvstop+407,
yycrank+1129,	yysvec+30,	yyvstop+409,
yycrank+1085,	yysvec+30,	yyvstop+411,
yycrank+1095,	yysvec+30,	yyvstop+414,
yycrank+1120,	yysvec+30,	yyvstop+416,
yycrank+1132,	yysvec+30,	yyvstop+418,
yycrank+1123,	yysvec+30,	yyvstop+420,
yycrank+1139,	yysvec+30,	yyvstop+422,
yycrank+1131,	yysvec+30,	yyvstop+424,
yycrank+1133,	yysvec+30,	yyvstop+426,
yycrank+1136,	yysvec+30,	yyvstop+428,
yycrank+1134,	yysvec+30,	yyvstop+430,
yycrank+1138,	yysvec+30,	yyvstop+432,
yycrank+1149,	yysvec+30,	yyvstop+434,
yycrank+1142,	yysvec+30,	yyvstop+436,
yycrank+1143,	yysvec+30,	yyvstop+438,
yycrank+1150,	yysvec+30,	yyvstop+440,
yycrank+1177,	yysvec+30,	yyvstop+442,
yycrank+1188,	yysvec+30,	yyvstop+444,
yycrank+1151,	yysvec+30,	yyvstop+446,
yycrank+1146,	yysvec+30,	yyvstop+448,
yycrank+1154,	yysvec+30,	yyvstop+451,
yycrank+1159,	yysvec+30,	yyvstop+454,
yycrank+1187,	yysvec+30,	yyvstop+457,
yycrank+1179,	yysvec+30,	yyvstop+459,
yycrank+1198,	yysvec+30,	yyvstop+462,
yycrank+1192,	yysvec+30,	yyvstop+464,
yycrank+1199,	yysvec+30,	yyvstop+466,
yycrank+1195,	yysvec+30,	yyvstop+468,
yycrank+1200,	yysvec+30,	yyvstop+470,
yycrank+1201,	yysvec+30,	yyvstop+472,
yycrank+1203,	yysvec+30,	yyvstop+474,
yycrank+1207,	yysvec+30,	yyvstop+476,
yycrank+1208,	yysvec+30,	yyvstop+478,
yycrank+1215,	yysvec+30,	yyvstop+480,
yycrank+1214,	yysvec+30,	yyvstop+482,
yycrank+1223,	yysvec+30,	yyvstop+484,
yycrank+1241,	yysvec+30,	yyvstop+486,
yycrank+1213,	yysvec+30,	yyvstop+488,
yycrank+1248,	yysvec+30,	yyvstop+491,
yycrank+1249,	yysvec+30,	yyvstop+493,
yycrank+1265,	yysvec+30,	yyvstop+495,
yycrank+1259,	yysvec+30,	yyvstop+497,
yycrank+1226,	yysvec+30,	yyvstop+499,
yycrank+1260,	yysvec+30,	yyvstop+502,
yycrank+1263,	yysvec+30,	yyvstop+504,
yycrank+1237,	yysvec+30,	yyvstop+506,
yycrank+1264,	yysvec+30,	yyvstop+509,
yycrank+1271,	yysvec+30,	yyvstop+511,
yycrank+1272,	yysvec+30,	yyvstop+513,
yycrank+1256,	yysvec+30,	yyvstop+515,
yycrank+1270,	yysvec+30,	yyvstop+518,
yycrank+1285,	yysvec+30,	yyvstop+520,
yycrank+1287,	yysvec+30,	yyvstop+522,
yycrank+1274,	yysvec+30,	yyvstop+524,
yycrank+1307,	yysvec+30,	yyvstop+527,
yycrank+1276,	yysvec+30,	yyvstop+529,
yycrank+1277,	yysvec+30,	yyvstop+531,
yycrank+1314,	yysvec+30,	yyvstop+533,
yycrank+1289,	yysvec+30,	yyvstop+535,
yycrank+1290,	yysvec+30,	yyvstop+538,
yycrank+1301,	yysvec+30,	yyvstop+540,
yycrank+1316,	yysvec+30,	yyvstop+542,
yycrank+1315,	yysvec+30,	yyvstop+544,
yycrank+1331,	yysvec+30,	yyvstop+547,
yycrank+1320,	yysvec+30,	yyvstop+549,
yycrank+1335,	yysvec+30,	yyvstop+552,
yycrank+1338,	yysvec+30,	yyvstop+554,
yycrank+1327,	yysvec+30,	yyvstop+556,
yycrank+1329,	yysvec+30,	yyvstop+558,
yycrank+1341,	yysvec+30,	yyvstop+560,
yycrank+1343,	yysvec+30,	yyvstop+562,
yycrank+1326,	yysvec+30,	yyvstop+564,
yycrank+1353,	yysvec+30,	yyvstop+567,
yycrank+1349,	yysvec+30,	yyvstop+569,
yycrank+1354,	yysvec+30,	yyvstop+571,
yycrank+1369,	yysvec+30,	yyvstop+573,
yycrank+1376,	yysvec+30,	yyvstop+575,
yycrank+1378,	yysvec+30,	yyvstop+577,
yycrank+1377,	yysvec+30,	yyvstop+579,
yycrank+1387,	yysvec+30,	yyvstop+581,
yycrank+1393,	yysvec+30,	yyvstop+583,
yycrank+1391,	yysvec+30,	yyvstop+585,
yycrank+1389,	yysvec+30,	yyvstop+587,
yycrank+1395,	yysvec+30,	yyvstop+589,
yycrank+1403,	yysvec+30,	yyvstop+591,
yycrank+1382,	yysvec+30,	yyvstop+593,
yycrank+1399,	yysvec+30,	yyvstop+596,
yycrank+1406,	yysvec+30,	yyvstop+598,
yycrank+1400,	yysvec+30,	yyvstop+600,
yycrank+1405,	yysvec+30,	yyvstop+603,
yycrank+1407,	yysvec+30,	yyvstop+605,
yycrank+1456,	yysvec+30,	yyvstop+608,
yycrank+1428,	yysvec+30,	yyvstop+610,
yycrank+1430,	yysvec+30,	yyvstop+612,
yycrank+1442,	yysvec+30,	yyvstop+614,
yycrank+1418,	yysvec+30,	yyvstop+616,
yycrank+1429,	yysvec+30,	yyvstop+619,
yycrank+1443,	yysvec+30,	yyvstop+622,
yycrank+1458,	yysvec+30,	yyvstop+624,
yycrank+1463,	yysvec+30,	yyvstop+626,
yycrank+1451,	yysvec+30,	yyvstop+628,
yycrank+1444,	yysvec+30,	yyvstop+630,
yycrank+1464,	yysvec+30,	yyvstop+633,
yycrank+1466,	yysvec+30,	yyvstop+635,
yycrank+1482,	yysvec+30,	yyvstop+637,
yycrank+1452,	yysvec+30,	yyvstop+639,
yycrank+1506,	yysvec+30,	yyvstop+642,
yycrank+1477,	yysvec+30,	yyvstop+644,
yycrank+1491,	yysvec+30,	yyvstop+646,
yycrank+1489,	yysvec+30,	yyvstop+648,
yycrank+1492,	yysvec+30,	yyvstop+650,
yycrank+1505,	yysvec+30,	yyvstop+652,
yycrank+1508,	yysvec+30,	yyvstop+654,
yycrank+1493,	yysvec+30,	yyvstop+656,
yycrank+1494,	yysvec+30,	yyvstop+659,
yycrank+1527,	yysvec+30,	yyvstop+662,
yycrank+1515,	yysvec+30,	yyvstop+664,
yycrank+1519,	yysvec+30,	yyvstop+666,
yycrank+1529,	yysvec+30,	yyvstop+668,
yycrank+1520,	yysvec+30,	yyvstop+670,
yycrank+1499,	yysvec+30,	yyvstop+672,
yycrank+1539,	yysvec+30,	yyvstop+675,
yycrank+1518,	yysvec+30,	yyvstop+677,
yycrank+1532,	yysvec+30,	yyvstop+680,
yycrank+1536,	yysvec+30,	yyvstop+682,
yycrank+1553,	yysvec+30,	yyvstop+684,
yycrank+1545,	yysvec+30,	yyvstop+686,
yycrank+1554,	yysvec+30,	yyvstop+688,
yycrank+1541,	yysvec+30,	yyvstop+690,
yycrank+1563,	yysvec+30,	yyvstop+692,
yycrank+1557,	yysvec+30,	yyvstop+694,
yycrank+1565,	yysvec+30,	yyvstop+696,
yycrank+1558,	yysvec+30,	yyvstop+698,
yycrank+1560,	yysvec+30,	yyvstop+701,
yycrank+1569,	yysvec+30,	yyvstop+704,
yycrank+1581,	yysvec+30,	yyvstop+706,
yycrank+1574,	yysvec+30,	yyvstop+708,
yycrank+1576,	yysvec+30,	yyvstop+710,
yycrank+1578,	yysvec+30,	yyvstop+712,
yycrank+1588,	yysvec+30,	yyvstop+715,
yycrank+1593,	yysvec+30,	yyvstop+717,
yycrank+1579,	yysvec+30,	yyvstop+719,
yycrank+1596,	yysvec+30,	yyvstop+721,
yycrank+1584,	yysvec+30,	yyvstop+723,
yycrank+1598,	yysvec+30,	yyvstop+725,
yycrank+1609,	yysvec+30,	yyvstop+727,
yycrank+1614,	yysvec+30,	yyvstop+729,
yycrank+1615,	yysvec+30,	yyvstop+731,
yycrank+1587,	yysvec+30,	yyvstop+733,
yycrank+1618,	yysvec+30,	yyvstop+736,
yycrank+1635,	yysvec+30,	yyvstop+738,
yycrank+1636,	yysvec+30,	yyvstop+740,
yycrank+1637,	yysvec+30,	yyvstop+742,
yycrank+1594,	yysvec+30,	yyvstop+744,
yycrank+1612,	yysvec+30,	yyvstop+747,
yycrank+1638,	yysvec+30,	yyvstop+750,
yycrank+1634,	yysvec+30,	yyvstop+752,
yycrank+1632,	yysvec+30,	yyvstop+754,
yycrank+1631,	yysvec+30,	yyvstop+757,
yycrank+1643,	yysvec+30,	yyvstop+760,
yycrank+1633,	yysvec+30,	yyvstop+762,
yycrank+1640,	yysvec+30,	yyvstop+764,
yycrank+1642,	yysvec+30,	yyvstop+766,
yycrank+1648,	yysvec+30,	yyvstop+769,
yycrank+1673,	yysvec+30,	yyvstop+772,
yycrank+1662,	yysvec+30,	yyvstop+774,
yycrank+1661,	yysvec+30,	yyvstop+776,
yycrank+1679,	yysvec+30,	yyvstop+778,
yycrank+1688,	yysvec+30,	yyvstop+780,
yycrank+1689,	yysvec+30,	yyvstop+782,
yycrank+1682,	yysvec+30,	yyvstop+784,
yycrank+1690,	yysvec+30,	yyvstop+786,
yycrank+1687,	yysvec+30,	yyvstop+788,
yycrank+1659,	yysvec+30,	yyvstop+790,
yycrank+1698,	yysvec+30,	yyvstop+792,
yycrank+1660,	yysvec+30,	yyvstop+794,
yycrank+1701,	yysvec+30,	yyvstop+796,
yycrank+1703,	yysvec+30,	yyvstop+798,
yycrank+1697,	yysvec+30,	yyvstop+800,
yycrank+1715,	yysvec+30,	yyvstop+803,
yycrank+1696,	yysvec+30,	yyvstop+805,
yycrank+1699,	yysvec+30,	yyvstop+808,
yycrank+1704,	yysvec+30,	yyvstop+811,
yycrank+1708,	yysvec+30,	yyvstop+813,
yycrank+1718,	yysvec+30,	yyvstop+815,
yycrank+1712,	yysvec+30,	yyvstop+817,
yycrank+1732,	yysvec+30,	yyvstop+819,
yycrank+1738,	yysvec+30,	yyvstop+821,
yycrank+1724,	yysvec+30,	yyvstop+823,
yycrank+1772,	yysvec+30,	yyvstop+825,
yycrank+1742,	yysvec+30,	yyvstop+827,
yycrank+1753,	yysvec+30,	yyvstop+829,
yycrank+1713,	yysvec+30,	yyvstop+831,
yycrank+1761,	yysvec+30,	yyvstop+834,
yycrank+1725,	yysvec+30,	yyvstop+836,
yycrank+1734,	yysvec+30,	yyvstop+839,
yycrank+1777,	yysvec+30,	yyvstop+841,
yycrank+1783,	yysvec+30,	yyvstop+843,
yycrank+1752,	yysvec+30,	yyvstop+845,
yycrank+1759,	yysvec+30,	yyvstop+848,
yycrank+1767,	yysvec+30,	yyvstop+850,
yycrank+1760,	yysvec+30,	yyvstop+852,
yycrank+1762,	yysvec+30,	yyvstop+855,
yycrank+1763,	yysvec+30,	yyvstop+857,
yycrank+1765,	yysvec+30,	yyvstop+860,
yycrank+1781,	yysvec+30,	yyvstop+863,
yycrank+1786,	yysvec+30,	yyvstop+865,
yycrank+1782,	yysvec+30,	yyvstop+867,
yycrank+1778,	yysvec+30,	yyvstop+869,
yycrank+1775,	yysvec+30,	yyvstop+871,
yycrank+1790,	yysvec+30,	yyvstop+874,
yycrank+1801,	yysvec+30,	yyvstop+876,
yycrank+1779,	yysvec+30,	yyvstop+878,
yycrank+1780,	yysvec+30,	yyvstop+881,
yycrank+1789,	yysvec+30,	yyvstop+883,
yycrank+1787,	yysvec+30,	yyvstop+885,
0,	0,	0};
struct yywork *yytop = yycrank+1915;
struct yysvf *yybgin = yysvec+1;
char yymatch[] ={
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,'"' ,01  ,'$' ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,'$' ,'$' ,'$' ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,01  ,01  ,01  ,01  ,'>' ,01  ,
01  ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,
'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,
'X' ,'Y' ,'J' ,01  ,01  ,01  ,01  ,'J' ,
01  ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,
'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,
'X' ,'Y' ,'J' ,01  ,01  ,01  ,01  ,01  ,
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
