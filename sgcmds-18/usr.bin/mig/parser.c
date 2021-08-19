# define sySkip 257
# define syRoutine 258
# define sySimpleRoutine 259
# define syCamelotRoutine 260
# define sySimpleProcedure 261
# define syProcedure 262
# define syFunction 263
# define sySubsystem 264
# define syKernel 265
# define syCamelot 266
# define syMsgType 267
# define syWaitTime 268
# define syNoWaitTime 269
# define syErrorProc 270
# define syServerPrefix 271
# define syUserPrefix 272
# define syRCSId 273
# define syImport 274
# define syUImport 275
# define sySImport 276
# define syIn 277
# define syOut 278
# define syInOut 279
# define syRequestPort 280
# define syReplyPort 281
# define syType 282
# define syArray 283
# define syStruct 284
# define syOf 285
# define syInTran 286
# define syOutTran 287
# define syDestructor 288
# define syCType 289
# define syCUserType 290
# define syCServerType 291
# define syColon 292
# define sySemi 293
# define syComma 294
# define syPlus 295
# define syMinus 296
# define syStar 297
# define syDiv 298
# define syLParen 299
# define syRParen 300
# define syEqual 301
# define syCaret 302
# define syTilde 303
# define syLAngle 304
# define syRAngle 305
# define syLBrack 306
# define syRBrack 307
# define syBar 308
# define syError 309
# define syNumber 310
# define sySymbolicType 311
# define syIdentifier 312
# define syString 313
# define syQString 314
# define syFileName 315
# define syIPCFlag 316

# line 104 "parser.y"

#include "lexxer.h"
#include "string.h"
#include "type.h"
#include "routine.h"
#include "statement.h"
#include "global.h"

static char *import_name();


# line 116 "parser.y"
typedef union 
{
    u_int number;
    identifier_t identifier;
    string_t string;
    statement_kind_t statement_kind;
    ipc_type_t *type;
    struct
    {
	u_int innumber;		/* msg_type_name value, when sending */
	string_t instr;
	u_int outnumber;	/* msg_type_name value, when receiving */
	string_t outstr;
	u_int size;		/* 0 means there is no default size */
    } symtype;
    routine_t *routine;
    arg_kind_t direction;
    argument_t *argument;
    ipc_flags_t flag;
} YYSTYPE;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 659 "parser.y"


yyerror(s)
    char *s;
{
    error(s);
}

static char *
import_name(sk)
    statement_kind_t sk;
{
    switch (sk)
    {
      case skImport:
	return "Import";
      case skSImport:
	return "SImport";
      case skUImport:
	return "UImport";
      default:
	fatal("import_name(%d): not import statement", (int) sk);
	/*NOTREACHED*/
    }
}
short yyexca[] ={
-1, 1,
	0, -1,
	267, 100,
	268, 100,
	273, 102,
	274, 101,
	275, 101,
	276, 101,
	-2, 0,
	};
# define YYNPROD 103
# define YYLAST 331
short yyact[]={

  99, 100, 196,  77,  78,  72,  73, 148,  75, 151,
  74, 205, 200, 198, 197, 192,  97, 147, 178,  94,
 150, 176, 131, 175, 161, 160, 159, 102, 103,  98,
 158, 157, 156, 102, 103, 130, 145, 144, 136, 135,
 117, 118, 119, 120, 121, 122,  68,  67,  66,  65,
 124,  64,  71,  63,  57,  55,  54,  53, 151, 123,
 168, 169, 170, 171, 168, 169, 170, 171,  87, 150,
 166, 132, 195, 203, 162, 204, 173, 168, 169, 170,
 171, 114, 113, 202, 189, 165,  76, 199, 133, 167,
 194, 108, 109, 110, 111, 112,  15,  11,  32,  33,
  34,  36,  35,  37,  31, 168, 169, 170, 171,  18,
  19,  20,  21, 189, 104, 168, 169, 170, 171, 146,
 187, 191,  22, 170, 171, 134, 190, 177,  80,  49,
  48,  47,  46,  14,  45,  44,  43,  42,  41,  40,
  39,  38, 193, 116, 143, 142, 141, 140, 139, 138,
 114, 113, 201, 188, 182, 180, 164,  62, 149, 174,
 108, 109, 110, 111, 112,  59,  60,  61,  51,  52,
 115, 101, 105,  89,  56,  96,  79,  30,  29,  17,
  70,  86,  69,  50,  16,  13,  12,   9,   8,   7,
   6,   5,   4,   3,   2,   1, 106, 107,  28,  27,
  26,  25,  24,  23,  10,  91,  90,  88,  95,  93,
  92,  58,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  81,  82,  83,  84,  85,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0, 125, 126, 127, 128,
   0,   0,   0, 129,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 152, 137,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 153, 163, 155, 154,   0,   0,
 172,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 179,   0, 181,   0, 183, 184, 185,
 186 };
short yypact[]={

-1000,-160,-1000,-152,-153,-154,-155,-156,-157,-158,
-159,-161,-162,-163,-1000,-164,-1000, -99,-1000,-255,
-256,-257,-258,-1000,-1000,-1000,-1000,-1000,-1000,-109,
-116,-1000,-259,-261,-263,-264,-265,-266,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-260,-303,-305,-1000,-1000,-1000,-1000,-215,-312,-1000,
-1000,-1000,-310,-171,-171,-171,-171,-171,-171,-242,
-1000,-1000,-1000,-1000,-1000,-1000,-283,-1000,-1000,-1000,
-186,-1000,-1000,-1000,-1000,-149,-1000,-1000,-246,-1000,
-1000,-1000,-283,-283,-283,-283,-1000,-277,-1000,-271,
-284,-237,-1000,-1000,-1000,-212,-168,-273,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-274,-143,-144,-145,
-146,-147,-148,-275,-276,-1000,-1000,-1000,-1000,-175,
-290,-241,-277,-1000,-117,-149,-215,-1000,-280,-281,
-282,-286,-287,-288,-230,-1000,-241,-129,-222,-218,
-1000,-241,-231,-1000,-1000,-1000,-289,-291,-172,-1000,
-1000,-1000,-294,-190,-1000,-130,-241,-131,-241,-241,
-241,-241,-180,-132,-181,-173,-178,-297,-150,-210,
-1000,-235,-1000,-174,-174,-1000,-1000,-1000,-1000,-314,
-298,-299,-213,-300,-1000,-133,-1000,-217,-227,-1000,
-228,-1000,-1000,-1000,-301,-1000 };
short yypgo[]={

   0, 211, 210, 209, 208, 158, 174, 207, 173, 206,
 205, 170, 171, 175, 204, 203, 202, 201, 200, 199,
 198, 197, 196, 176, 172, 159, 195, 194, 193, 192,
 191, 190, 189, 188, 187, 186, 185, 184, 183, 182,
 181, 180, 179, 178, 177 };
short yyr1[]={

   0,  26,  26,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  28,  37,  38,  38,
  41,  41,  39,  40,  30,  29,  29,  31,  32,  33,
  35,   1,   1,   1,  36,  34,   6,   7,   7,   7,
   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,
   8,   8,   8,   9,   9,  25,  25,  12,  12,  13,
  13,  10,   2,   2,   2,   3,   4,   5,   5,   5,
   5,   5,   5,  14,  14,  14,  14,  14,  14,  15,
  16,  17,  18,  19,  20,  23,  23,  24,  24,  22,
  21,  21,  21,  21,  21,  21,  21,  21,  11,  11,
  42,  43,  44 };
short yyr2[]={

   0,   0,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   1,   2,   4,   1,   0,   2,
   1,   1,   1,   1,   3,   3,   1,   2,   2,   2,
   3,   1,   1,   1,   3,   2,   3,   1,   8,   8,
   7,   4,   4,   4,   7,   9,   3,   1,   1,   2,
   2,   2,   2,   1,   6,   0,   3,   1,   1,   1,
   3,   1,   4,   5,   7,   5,   5,   3,   3,   3,
   3,   1,   3,   1,   1,   1,   1,   1,   1,   3,
   3,   3,   3,   3,   4,   2,   3,   1,   3,   4,
   0,   1,   1,   1,   1,   1,   1,   1,   2,   2,
   0,   0,   0 };
short yychk[]={

-1000, -26, -27, -28, -29, -30, -31, -32, -33, -34,
 -14, 257, -35, -36, 293, 256, -37, -42, 269, 270,
 271, 272, 282, -15, -16, -17, -18, -19, -20, -43,
 -44, 264, 258, 259, 260, 262, 261, 263, 293, 293,
 293, 293, 293, 293, 293, 293, 293, 293, 293, 293,
 -38, 267, 268, 312, 312, 312,  -6, 312,  -1, 274,
 275, 276, 273, 312, 312, 312, 312, 312, 312, -39,
 -41, 312, 265, 266, 313, 313, 301, 315, 314, -23,
 299, -23, -23, -23, -23, -23, -40, 310,  -7,  -8,
  -9, -10,  -2,  -3, 302,  -4, -13, 299, 312, 283,
 284, -12, 310, 311, 300, -24, -22, -21, 277, 278,
 279, 280, 281, 268, 267, -11, 292, 286, 287, 288,
 289, 290, 291, 305, 296,  -8,  -8,  -8,  -8, -13,
 306, 306, 308, 300, 293, 312, 312,  -6, 292, 292,
 292, 292, 292, 292, 312, 312, 294, 307, 297,  -5,
 310, 299,  -5, -12, -24, -11, 312, 312, 312, 312,
 312, 312, 304,  -5, 285, 307, 292, 307, 295, 296,
 297, 298,  -5, 307, -25, 312, 312, 299, 312, -25,
 285,  -5, 285,  -5,  -5,  -5,  -5, 300, 285, 294,
 299, 299, 312, 292, 300, 307, 316, 312, 312, 300,
 312, 285, 300, 300, 303, 312 };
short yydef[]={

   1,  -2,   2,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  14,   0,  18,   0,  26,   0,
   0,   0,   0,  73,  74,  75,  76,  77,  78,   0,
   0,  17,   0,   0,   0,   0,   0,   0,   3,   4,
   5,   6,   7,   8,   9,  10,  11,  12,  13,  15,
   0,   0,   0,  27,  28,  29,  35,   0,   0,  31,
  32,  33,   0,   0,   0,   0,   0,   0,   0,   0,
  19,  22,  20,  21,  24,  25,   0,  30,  34,  79,
  90,  80,  81,  82,  83,   0,  16,  23,  36,  37,
  47,  48,   0,   0,   0,   0,  53,   0,  61,   0,
   0,  59,  57,  58,  85,   0,  87,   0,  91,  92,
  93,  94,  95,  96,  97,  84,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  49,  50,  51,  52,   0,
   0,   0,   0,  86,  90,   0,  98,  99,   0,   0,
   0,   0,   0,   0,   0,  46,   0,   0,   0,   0,
  71,   0,   0,  60,  88,  55,   0,   0,   0,  41,
  42,  43,   0,  55,  62,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  89,   0,   0,   0,   0,   0,
  63,   0,  65,  67,  68,  69,  70,  72,  66,   0,
   0,   0,   0,   0,  54,   0,  56,   0,   0,  40,
  44,  64,  38,  39,   0,  45 };
# line 1 "/usr/lib/yaccpar"
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif not lint

# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps>= &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 10:
# line 151 "parser.y"
{
    register statement_t *st = stAlloc();

    st->stKind = skRoutine;
    st->stRoutine = yypvt[-1].routine;
    rtCheckRoutine(yypvt[-1].routine);
    if (BeVerbose)
	rtPrintRoutine(yypvt[-1].routine);
} break;
case 11:
# line 161 "parser.y"
{ rtSkip(); } break;
case 15:
# line 166 "parser.y"
{ yyerrok; } break;
case 16:
# line 171 "parser.y"
{
    if (BeVerbose)
    {
	printf("Subsystem %s: base = %u, IsKernel = %s, IsCamelot = %s\n\n",
	       SubsystemName, SubsystemBase,
	       strbool(IsKernel), strbool(IsCamelot));
    }
} break;
case 17:
# line 182 "parser.y"
{
    if (SubsystemName != strNULL)
    {
	warn("previous Subsystem decl (of %s) will be ignored", SubsystemName);
	IsCamelot = FALSE;
	IsKernel = FALSE;
	strfree(SubsystemName);
    }
} break;
case 20:
# line 198 "parser.y"
{
    if (IsKernel)
	warn("duplicate Kernel keyword");
    warn("the Kernel subsystem keyword is obsolete");
    IsKernel = TRUE;
} break;
case 21:
# line 205 "parser.y"
{
    if (IsCamelot)
	warn("duplicate Camelot keyword");
    IsCamelot = TRUE;
} break;
case 22:
# line 212 "parser.y"
{ SubsystemName = yypvt[-0].identifier; } break;
case 23:
# line 215 "parser.y"
{ SubsystemBase = yypvt[-0].number; } break;
case 24:
# line 219 "parser.y"
{
	if (streql(yypvt[-0].string,"MSG_TYPE_NORMAL"))
	    MsgType = strNULL;
	else
	    MsgType = yypvt[-0].string;
	if (BeVerbose)
	    printf("MsgType %s\n\n",yypvt[-0].string);
} break;
case 25:
# line 230 "parser.y"
{
    WaitTime = yypvt[-0].string;
    if (BeVerbose)
	printf("WaitTime %s\n\n", WaitTime);
} break;
case 26:
# line 236 "parser.y"
{
    WaitTime = strNULL;
    if (BeVerbose)
	printf("NoWaitTime\n\n");
} break;
case 27:
# line 244 "parser.y"
{
    ErrorProc = yypvt[-0].identifier;
    if (BeVerbose)
	printf("ErrorProc %s\n\n", ErrorProc);
} break;
case 28:
# line 252 "parser.y"
{
    ServerPrefix = yypvt[-0].identifier;
    if (BeVerbose)
	printf("ServerPrefix %s\n\n", ServerPrefix);
} break;
case 29:
# line 260 "parser.y"
{
    UserPrefix = yypvt[-0].identifier;
    if (BeVerbose)
	printf("UserPrefix %s\n\n", UserPrefix);
} break;
case 30:
# line 268 "parser.y"
{
    register statement_t *st = stAlloc();
    st->stKind = yypvt[-1].statement_kind;
    st->stFileName = yypvt[-0].string;

    if (BeVerbose)
	printf("%s %s\n\n", import_name(yypvt[-1].statement_kind), yypvt[-0].string);
} break;
case 31:
# line 278 "parser.y"
{ yyval.statement_kind = skImport; } break;
case 32:
# line 279 "parser.y"
{ yyval.statement_kind = skUImport; } break;
case 33:
# line 280 "parser.y"
{ yyval.statement_kind = skSImport; } break;
case 34:
# line 284 "parser.y"
{
    if (RCSId != strNULL)
	warn("previous RCS decl will be ignored");
    if (BeVerbose)
	printf("RCSId %s\n\n", yypvt[-0].string);
    RCSId = yypvt[-0].string;
} break;
case 35:
# line 294 "parser.y"
{
    register identifier_t name = yypvt[-0].type->itName;

    if (itLookUp(name) != itNULL)
	warn("overriding previous definition of %s", name);
    itInsert(name, yypvt[-0].type);
} break;
case 36:
# line 304 "parser.y"
{ itTypeDecl(yypvt[-2].identifier, yyval.type = yypvt[-0].type); } break;
case 37:
# line 308 "parser.y"
{ yyval.type = itResetType(yypvt[-0].type); } break;
case 38:
# line 311 "parser.y"
{
    yyval.type = yypvt[-7].type;

    if ((yyval.type->itTransType != strNULL) && !streql(yyval.type->itTransType, yypvt[-4].identifier))
	warn("conflicting translation types (%s, %s)",
	     yyval.type->itTransType, yypvt[-4].identifier);
    yyval.type->itTransType = yypvt[-4].identifier;

    if ((yyval.type->itInTrans != strNULL) && !streql(yyval.type->itInTrans, yypvt[-3].identifier))
	warn("conflicting in-translation functions (%s, %s)",
	     yyval.type->itInTrans, yypvt[-3].identifier);
    yyval.type->itInTrans = yypvt[-3].identifier;

    if ((yyval.type->itServerType != strNULL) && !streql(yyval.type->itServerType, yypvt[-1].identifier))
	warn("conflicting server types (%s, %s)",
	     yyval.type->itServerType, yypvt[-1].identifier);
    yyval.type->itServerType = yypvt[-1].identifier;
} break;
case 39:
# line 331 "parser.y"
{
    yyval.type = yypvt[-7].type;

    if ((yyval.type->itServerType != strNULL) && !streql(yyval.type->itServerType, yypvt[-4].identifier))
	warn("conflicting server types (%s, %s)",
	     yyval.type->itServerType, yypvt[-4].identifier);
    yyval.type->itServerType = yypvt[-4].identifier;

    if ((yyval.type->itOutTrans != strNULL) && !streql(yyval.type->itOutTrans, yypvt[-3].identifier))
	warn("conflicting out-translation functions (%s, %s)",
	     yyval.type->itOutTrans, yypvt[-3].identifier);
    yyval.type->itOutTrans = yypvt[-3].identifier;

    if ((yyval.type->itTransType != strNULL) && !streql(yyval.type->itTransType, yypvt[-1].identifier))
	warn("conflicting translation types (%s, %s)",
	     yyval.type->itTransType, yypvt[-1].identifier);
    yyval.type->itTransType = yypvt[-1].identifier;
} break;
case 40:
# line 351 "parser.y"
{
    yyval.type = yypvt[-6].type;

    if ((yyval.type->itDestructor != strNULL) && !streql(yyval.type->itDestructor, yypvt[-3].identifier))
	warn("conflicting destructor functions (%s, %s)",
	     yyval.type->itDestructor, yypvt[-3].identifier);
    yyval.type->itDestructor = yypvt[-3].identifier;

    if ((yyval.type->itTransType != strNULL) && !streql(yyval.type->itTransType, yypvt[-1].identifier))
	warn("conflicting translation types (%s, %s)",
	     yyval.type->itTransType, yypvt[-1].identifier);
    yyval.type->itTransType = yypvt[-1].identifier;
} break;
case 41:
# line 365 "parser.y"
{
    yyval.type = yypvt[-3].type;

    if ((yyval.type->itUserType != strNULL) && !streql(yyval.type->itUserType, yypvt[-0].identifier))
	warn("conflicting user types (%s, %s)",
	     yyval.type->itUserType, yypvt[-0].identifier);
    yyval.type->itUserType = yypvt[-0].identifier;

    if ((yyval.type->itServerType != strNULL) && !streql(yyval.type->itServerType, yypvt[-0].identifier))
	warn("conflicting server types (%s, %s)",
	     yyval.type->itServerType, yypvt[-0].identifier);
    yyval.type->itServerType = yypvt[-0].identifier;
} break;
case 42:
# line 379 "parser.y"
{
    yyval.type = yypvt[-3].type;

    if ((yyval.type->itUserType != strNULL) && !streql(yyval.type->itUserType, yypvt[-0].identifier))
	warn("conflicting user types (%s, %s)",
	     yyval.type->itUserType, yypvt[-0].identifier);
    yyval.type->itUserType = yypvt[-0].identifier;
} break;
case 43:
# line 389 "parser.y"
{
    yyval.type = yypvt[-3].type;

    if ((yyval.type->itServerType != strNULL) && !streql(yyval.type->itServerType, yypvt[-0].identifier))
	warn("conflicting server types (%s, %s)",
	     yyval.type->itServerType, yypvt[-0].identifier);
    yyval.type->itServerType = yypvt[-0].identifier;
} break;
case 44:
# line 399 "parser.y"
{
    warn("obsolete translation spec");
    yyval.type = yypvt[-6].type;

    if ((yyval.type->itInTrans != strNULL) && !streql(yyval.type->itInTrans, yypvt[-4].identifier))
	warn("conflicting in-translation functions (%s, %s)",
	     yyval.type->itInTrans, yypvt[-4].identifier);
    yyval.type->itInTrans = yypvt[-4].identifier;

    if ((yyval.type->itOutTrans != strNULL) && !streql(yyval.type->itOutTrans, yypvt[-2].identifier))
	warn("conflicting out-translation functions (%s, %s)",
	     yyval.type->itOutTrans, yypvt[-2].identifier);
    yyval.type->itOutTrans = yypvt[-2].identifier;

    if ((yyval.type->itTransType != strNULL) && !streql(yyval.type->itTransType, yypvt[-0].identifier))
	warn("conflicting translation types (%s, %s)",
	     yyval.type->itTransType, yypvt[-0].identifier);
    yyval.type->itTransType = yypvt[-0].identifier;
} break;
case 45:
# line 421 "parser.y"
{
    warn("obsolete translation spec");
    yyval.type = yypvt[-8].type;

    if ((yyval.type->itInTrans != strNULL) && !streql(yyval.type->itInTrans, yypvt[-6].identifier))
	warn("conflicting in-translation functions (%s, %s)",
	     yyval.type->itInTrans, yypvt[-6].identifier);
    yyval.type->itInTrans = yypvt[-6].identifier;

    if ((yyval.type->itOutTrans != strNULL) && !streql(yyval.type->itOutTrans, yypvt[-4].identifier))
	warn("conflicting out-translation functions (%s, %s)",
	     yyval.type->itOutTrans, yypvt[-4].identifier);
    yyval.type->itOutTrans = yypvt[-4].identifier;

    if ((yyval.type->itTransType != strNULL) && !streql(yyval.type->itTransType, yypvt[-2].identifier))
	warn("conflicting translation types (%s, %s)",
	     yyval.type->itTransType, yypvt[-2].identifier);
    yyval.type->itTransType = yypvt[-2].identifier;

    if ((yyval.type->itServerType != strNULL) && !streql(yyval.type->itServerType, yypvt[-0].identifier))
	warn("conflicting server types (%s, %s)",
	     yyval.type->itServerType, yypvt[-0].identifier);
    yyval.type->itServerType = yypvt[-0].identifier;
} break;
case 46:
# line 446 "parser.y"
{
    warn("obsolete destructor spec");
    yyval.type = yypvt[-2].type;

    if ((yyval.type->itDestructor != strNULL) && !streql(yyval.type->itDestructor, yypvt[-0].identifier))
	warn("conflicting destructor functions (%s, %s)",
	     yyval.type->itDestructor, yypvt[-0].identifier);
    yyval.type->itDestructor = yypvt[-0].identifier;
} break;
case 47:
# line 458 "parser.y"
{ yyval.type = yypvt[-0].type; } break;
case 48:
# line 460 "parser.y"
{ yyval.type = yypvt[-0].type; } break;
case 49:
# line 462 "parser.y"
{ yyval.type = itVarArrayDecl(yypvt[-1].number, yypvt[-0].type); } break;
case 50:
# line 464 "parser.y"
{ yyval.type = itArrayDecl(yypvt[-1].number, yypvt[-0].type); } break;
case 51:
# line 466 "parser.y"
{ yyval.type = itPtrDecl(yypvt[-0].type); } break;
case 52:
# line 468 "parser.y"
{ yyval.type = itStructDecl(yypvt[-1].number, yypvt[-0].type); } break;
case 53:
# line 472 "parser.y"
{
    yyval.type = itShortDecl(yypvt[-0].symtype.innumber, yypvt[-0].symtype.instr,
		     yypvt[-0].symtype.outnumber, yypvt[-0].symtype.outstr,
		     yypvt[-0].symtype.size);
} break;
case 54:
# line 479 "parser.y"
{
    yyval.type = itLongDecl(yypvt[-4].symtype.innumber, yypvt[-4].symtype.instr,
		    yypvt[-4].symtype.outnumber, yypvt[-4].symtype.outstr,
		    yypvt[-4].symtype.size, yypvt[-2].number, yypvt[-1].flag);
} break;
case 55:
# line 487 "parser.y"
{ yyval.flag = flNone; } break;
case 56:
# line 489 "parser.y"
{
    if (yypvt[-2].flag & yypvt[-0].flag)
	warn("redundant IPC flag ignored");
    else
	yyval.flag = yypvt[-2].flag | yypvt[-0].flag;
} break;
case 57:
# line 498 "parser.y"
{
    yyval.symtype.innumber = yyval.symtype.outnumber = yypvt[-0].number;
    yyval.symtype.instr = yyval.symtype.outstr = strNULL;
    yyval.symtype.size = 0;
} break;
case 58:
# line 504 "parser.y"
{ yyval.symtype = yypvt[-0].symtype; } break;
case 59:
# line 508 "parser.y"
{ yyval.symtype = yypvt[-0].symtype; } break;
case 60:
# line 510 "parser.y"
{
    if (yypvt[-2].symtype.size != yypvt[-0].symtype.size)
    {
	if (yypvt[-2].symtype.size == 0)
	    yyval.symtype.size = yypvt[-0].symtype.size;
	else if (yypvt[-0].symtype.size == 0)
	    yyval.symtype.size = yypvt[-2].symtype.size;
	else
	{
	    error("sizes in IPCTypes (%d, %d) aren't equal",
		  yypvt[-2].symtype.size, yypvt[-0].symtype.size);
	    yyval.symtype.size = 0;
	}
    }
    else
	yyval.symtype.size = yypvt[-2].symtype.size;
    yyval.symtype.innumber = yypvt[-2].symtype.innumber;
    yyval.symtype.instr = yypvt[-2].symtype.instr;
    yyval.symtype.outnumber = yypvt[-0].symtype.outnumber;
    yyval.symtype.outstr = yypvt[-0].symtype.outstr;
} break;
case 61:
# line 534 "parser.y"
{ yyval.type = itPrevDecl(yypvt[-0].identifier); } break;
case 62:
# line 538 "parser.y"
{ yyval.number = 0; } break;
case 63:
# line 540 "parser.y"
{ yyval.number = 0; } break;
case 64:
# line 543 "parser.y"
{ yyval.number = yypvt[-2].number; } break;
case 65:
# line 547 "parser.y"
{ yyval.number = yypvt[-2].number; } break;
case 66:
# line 551 "parser.y"
{ yyval.number = yypvt[-2].number; } break;
case 67:
# line 555 "parser.y"
{ yyval.number = yypvt[-2].number + yypvt[-0].number;	} break;
case 68:
# line 557 "parser.y"
{ yyval.number = yypvt[-2].number - yypvt[-0].number;	} break;
case 69:
# line 559 "parser.y"
{ yyval.number = yypvt[-2].number * yypvt[-0].number;	} break;
case 70:
# line 561 "parser.y"
{ yyval.number = yypvt[-2].number / yypvt[-0].number;	} break;
case 71:
# line 563 "parser.y"
{ yyval.number = yypvt[-0].number;	} break;
case 72:
# line 565 "parser.y"
{ yyval.number = yypvt[-1].number;	} break;
case 73:
# line 569 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 74:
# line 570 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 75:
# line 571 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 76:
# line 572 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 77:
# line 573 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 78:
# line 574 "parser.y"
{ yyval.routine = yypvt[-0].routine; } break;
case 79:
# line 578 "parser.y"
{ yyval.routine = rtMakeRoutine(yypvt[-1].identifier, yypvt[-0].argument); } break;
case 80:
# line 582 "parser.y"
{ yyval.routine = rtMakeSimpleRoutine(yypvt[-1].identifier, yypvt[-0].argument); } break;
case 81:
# line 586 "parser.y"
{ yyval.routine = rtMakeCamelotRoutine(yypvt[-1].identifier, yypvt[-0].argument); } break;
case 82:
# line 590 "parser.y"
{ yyval.routine = rtMakeProcedure(yypvt[-1].identifier, yypvt[-0].argument); } break;
case 83:
# line 594 "parser.y"
{ yyval.routine = rtMakeSimpleProcedure(yypvt[-1].identifier, yypvt[-0].argument); } break;
case 84:
# line 598 "parser.y"
{ yyval.routine = rtMakeFunction(yypvt[-2].identifier, yypvt[-1].argument, yypvt[-0].type); } break;
case 85:
# line 602 "parser.y"
{ yyval.argument = argNULL; } break;
case 86:
# line 604 "parser.y"
{ yyval.argument = yypvt[-1].argument; } break;
case 87:
# line 609 "parser.y"
{ yyval.argument = yypvt[-0].argument; } break;
case 88:
# line 611 "parser.y"
{
    yyval.argument = yypvt[-2].argument;
    yyval.argument->argNext = yypvt[-0].argument;
} break;
case 89:
# line 618 "parser.y"
{
    yyval.argument = argAlloc();
    yyval.argument->argKind = yypvt[-3].direction;
    yyval.argument->argName = yypvt[-2].identifier;
    yyval.argument->argType = yypvt[-1].type;
    yyval.argument->argFlags = yypvt[-0].flag;
} break;
case 90:
# line 627 "parser.y"
{ yyval.direction = akNone; } break;
case 91:
# line 628 "parser.y"
{ yyval.direction = akIn; } break;
case 92:
# line 629 "parser.y"
{ yyval.direction = akOut; } break;
case 93:
# line 630 "parser.y"
{ yyval.direction = akInOut; } break;
case 94:
# line 631 "parser.y"
{ yyval.direction = akRequestPort; } break;
case 95:
# line 632 "parser.y"
{ yyval.direction = akReplyPort; } break;
case 96:
# line 633 "parser.y"
{ yyval.direction = akWaitTime; } break;
case 97:
# line 634 "parser.y"
{ yyval.direction = akMsgType; } break;
case 98:
# line 638 "parser.y"
{
    yyval.type = itLookUp(yypvt[-0].identifier);
    if (yyval.type == itNULL)
	error("type '%s' not defined", yypvt[-0].identifier);
} break;
case 99:
# line 644 "parser.y"
{ yyval.type = yypvt[-0].type; } break;
case 100:
# line 648 "parser.y"
{ LookString(); } break;
case 101:
# line 652 "parser.y"
{ LookFileName(); } break;
case 102:
# line 656 "parser.y"
{ LookQString(); } break;
# line 148 "/usr/lib/yaccpar"

		}
		goto yystack;  /* stack new state and value */

	}
