# define NUMBER 257
# define NUMBER4 258
# define WEEKDAY 259
# define MONTH 260
# define HOUR 261
# define SHOUR 262
# define TODAY 263
# define NOW 264
# define TONIGHT 265
# define NEXT 266
# define THIS 267
# define DAY 268
# define WEEK 269
# define FORTNIGHT 270
# define UPCOMING 271
# define EVERY 272
# define FROM 273
# define BEFORE 274
# define THE 275
# define A 276
# define AT 277
# define ON 278
# define LAST 279
# define AFTER 280
# define IN 281
# define OF 282
# define AGO 283
# define WORD_MONTH 284
# define NOON 285
# define AMPM 286
# define TIMEKEY 287
# define AND 288
# define NWORD 289
# define NTHWORD 290
# define ST 291
# define ND 292
# define RD 293
# define TH 294
# define CHRISTMAS 295
# define NEW 296
# define YEAR 297
# define ALL 298
# define FOOLS 299
# define JUNK 300

# line 89 "compat-cmucs/parsedate.y"
#include <sys/types.h>
#include <setjmp.h>
#include <sys/time.h>
#include <c.h>

# define MAXPEEP 3
# define NDTMS 10
# define NOYEAR -1901
/* PTM: indicates ptm */
# define PTM -1
/* CURRTM: indicates current time */
# define CURRTM -2
/* CURRDATE: indicates current date (time set to -1). */
# define CURRDATE -3
/* PAST, FUTURE: logical values for past and future */
# define PAST 1
# define FUTURE 0

static int junk_err;			/* junk token give error? */
static int ppf;				/* past, or future */
static int nottoday;			/* future doesn't include today */
static struct tm scurrtm;
static struct tm *currtm = &scurrtm;	/* current time */
static char *strp;			/* current position in date string */
static int delim;			/* previous field delimiter */
static jmp_buf errbuf;			/* jump location for parse errors */
extern char *nxtarg();
struct stable { char *text;  int token;  int lexval; };
static int shour, tmkey;
static struct tm ptm;
static int pcount;
static int result;

/* Date structure for constraining dates */
struct dtm 
{ int repn; 				     /* Current representation */
  int days;
  struct tm tm;
  int count;
};

static struct dtm dtm[NDTMS];
static int dtmused[NDTMS];

/* Representations */
# define RDAYS 0
# define RTM 1

/*
 *  Month and week day names (upper case only) and other words.
 */
struct stable strings[] =
{
  { "JAN*UARY", MONTH, 0 },		/* months (0-11) */
  { "FEB*RUARY", MONTH, 1 },
  { "MAR*CH", MONTH, 2 },
  { "APR*IL", MONTH, 3 },
  { "MAY", MONTH, 4 },
  { "JUN*E", MONTH, 5 },
  { "JUL*Y", MONTH, 6 },
  { "AUG*UST", MONTH, 7 },
  { "SEP*TEMBER", MONTH, 8 },
  { "OCT*OBER", MONTH, 9 },
  { "NOV*EMBER", MONTH, 10 },
  { "DEC*EMBER", MONTH, 11 },
  { "SUN*DAY", WEEKDAY, 0 },	/* days of the week (0-6) */
  { "MON*DAY", WEEKDAY, 1 },
  { "TUE*SDAY", WEEKDAY, 2 },
  { "WED*NESDAY", WEEKDAY, 3 },
  { "THU*RSDAY", WEEKDAY, 4 },
  { "FRI*DAY", WEEKDAY, 5 },
  { "SAT*URDAY", WEEKDAY, 6 },
  { "YESTERDAY", TODAY, -1 },	/* relative to today */
  { "TODAY", TODAY, 0 },
  { "TONIGHT", TONIGHT, 0 },
  { "NOW", NOW, 0 },
  { "AGO", AGO, 0 },
  { "TOMORROW", TODAY, 1 },
  { "NEXT", NEXT, 0 },		/* keywords */
  { "THIS", THIS, 0 },
  { "UPCOMING", UPCOMING, 0 },
  { "DAY*S", DAY, 0 },
  { "WEEK*S", WEEK, 0 },
  { "MONTH*S", WORD_MONTH, 0 },
  { "YEAR*S", YEAR, 0 },
  { "ALL", ALL, 0 },
  { "FOOL*S", FOOLS, 0 },
  { "FORTNIGHT", FORTNIGHT, 0 },	/* two weeks (Australian) */
/*  { "EVERY", EVERY, 0 }, */
  { "FROM", FROM, 0 },
  { "AFTER", AFTER, 0 },
  { "BEFORE", BEFORE, 0 },
  { "LAST", LAST, 0 },
  { "THE", THE, 0 },
  { "A", A, 0 },
  { "AT", AT, 0 },
  { "ON", ON, 0 },
  { "IN", IN, 0 },
  { "OF", OF, 0 },
/*  { "AND", AND, 0 }, */
  { "MORNING", TIMEKEY, 0 },	/* time keywords. Morning is 0:00 - 11:59 */
  { "AFTERNOON", TIMEKEY, 12 }, /* Afternoon is 12:00 - 23:59 */
  { "EVENING", TIMEKEY, 15 },	/* Evening is 15:00 - 02:59 */
  { "NIGHT", TIMEKEY, 17 },	/* Night is 17:00 - 04:59 */
  { "NOON", NOON, 12 },		/* time specifications */
  { "MIDNIGHT", NOON, 24 },
  { "ONE", NWORD, 1 },		/* numbers up to 19 */
  { "TWO", NWORD, 2 },
  { "THREE", NWORD, 3 },
  { "FOUR", NWORD, 4 },
  { "FIVE", NWORD, 5 },
  { "SIX", NWORD, 6 },
  { "SEVEN", NWORD, 7 },
  { "EIGHT", NWORD, 8 },
  { "NINE", NWORD, 9 },
  { "TEN", NWORD, 10 },
  { "ELEVEN", NWORD, 11 },
  { "TWELVE", NWORD, 12 },
  { "THIRTEEN", NWORD, 13 },
  { "FOURTEEN", NWORD, 14 },
  { "FIFTEEN", NWORD, 15 },
  { "SIXTEEN", NWORD, 16 },
  { "SEVENTEEN", NWORD, 17 },
  { "EIGHTEEN", NWORD, 18 },
  { "NINETEEN", NWORD, 19 },
  { "FIRST", NTHWORD, 1 },		/* number up to 19th */
  { "SECOND", NTHWORD, 2 },
  { "THIRD", NTHWORD, 3 },
  { "FOURTH", NTHWORD, 4 },
  { "FIFTH", NTHWORD, 5 },
  { "SIXTH", NTHWORD, 6 },
  { "SEVENTH", NTHWORD, 7 },
  { "EIGHT", NTHWORD, 8 },
  { "NINTH", NTHWORD, 9 },
  { "TENTH", NTHWORD, 10 },
  { "ELEVENTH", NTHWORD, 11 },
  { "TWELFTH", NTHWORD, 12 },
  { "THIRTEENTH", NTHWORD, 13 },
  { "FOURTEENTH", NTHWORD, 14 },
  { "FIFTEENTH", NTHWORD, 15 },
  { "SIXTEENTH", NTHWORD, 16 },
  { "SEVENTEENTH", NTHWORD, 17 },
  { "EIGHTEENTH", NTHWORD, 18 },
  { "NINETEENTH", NTHWORD, 19 },
  { "ST", ST, 0 },		/* for 1st */
  { "ND", ND, 0 },		/* 2nd */
  { "RD", RD, 0 },		/* 3rd */
  { "TH", TH, 0 },		/* nth */
  { "AM", AMPM, 0 },		/* time qualifiers */
  { "A.M.", AMPM, 0 },
  { "PM", AMPM, 12 },
  { "P.M.", AMPM, 12 },
  { "CHRISTMAS", CHRISTMAS, 1225 },	/* special dates */
  { "NEW", NEW, 101 },
  { 0, 0, 0 }
};

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 706 "compat-cmucs/parsedate.y"


parsedate (str, tmp, settm, select, err)
char *str;
struct tm *tmp;
int settm, select, err;
{   long time (), curtim;
    struct tm *localtime ();
    char tstr[81];
    int i;
    register struct tm *rtm;

    junk_err = err;
    if (settm)
    { curtim = time (0);
      *currtm = *localtime (&curtim);
    }
    else
    { *currtm = *tmp;
    }

    /*  initialize lexical analyzer  */
    for (i = 0; i < NDTMS; i++)
      dtmused[i] = 0;
    strncpy(strp = tstr, str, 80);
    tstr[80] = 0;
    ptm.tm_year = NOYEAR;
    ptm.tm_mon = -1;
    ptm.tm_mday = -1;
    ptm.tm_wday = -1;
    ptm.tm_yday = -1;
    ptm.tm_hour = -1;
    ptm.tm_min = -1;
    ptm.tm_sec = -1;
    pcount = 0;
    currtm->tm_yday = currtm->tm_wday = -1;
    shour = -1;
    tmkey = -1;
    delim = 0;
    ppf = select;
    nottoday = 0;

    if (setjmp(errbuf) == 0)
    {
	yyparse();
	rtm = &(dtm[result].tm);
	setrep (&dtm[result], RTM);
	rtm->tm_hour = ptm.tm_hour;
	rtm->tm_min = ptm.tm_min;
	rtm->tm_sec = ptm.tm_sec;
	time_def (rtm, currtm);
	/* If time is 24:00 or later, advance date to next day. */
	if (rtm->tm_hour >= 24)
	{ incr (result, rtm->tm_hour / 24);
	  setrep (&dtm[result], RTM);
	  rtm->tm_hour = rtm->tm_hour % 24;
	}
	if (dtm[result].repn == RTM && dtm[result].tm.tm_yday == -1)
	  setrep (&dtm[result], RDAYS);
	setrep (&dtm[result], RTM);
	*tmp = dtm[result].tm;
	return(0);			/* return here on successful parse */
    }
    else
	return(CERROR);			/* return here on error */

}

/*
 *  yyerror - error routine (called by yyparse)
 *
 *     Performs a jump to the error return location established
 *  by pdate().
 */

static yyerror()
{

    longjmp(errbuf, 1);

}

static int prelval;
static struct { int token; int val; } peep[MAXPEEP];
static int peepb = 0, peepe = 0;
static int lead0;

/*  yylex - return next token in date string.
 * 
 *  Obtains the tokens from nextlex() and returns them. Checks for
 *  NUMBER tokens which are really HOURs. This is done by peeping ahead.
 *  It gives more lookahead than yacc can support.
 */

static int yylex ()
{ register int ctoken, htoken;
  ctoken = nextlex();
  htoken = lead0 ? HOUR : SHOUR;
  if (ctoken == NUMBER && yylval <= 24)
  { /* Possible hour - check it out */
    peeper (1); 			     /* Allow 1 token peeping */
    if (peep[0].token == AMPM || peep[0].token == NOON)
      return (htoken); 			     /* NN AM or NN PM or 12 NOON */
    if (peep[0].token == ':' || peep[0].token == '.')
    { peeper (2);
      if (peep[1].token == NUMBER)
	return (htoken);		     /* NN:NN or NN.NN */
    }
  }
  return (ctoken);
}

static int nextlex ()
{ register int token;
  if (peepb < peepe)
  { token = peep[peepb].token;
    yylval = peep[peepb].val;
    peepb++;
  }
  else
  { token = prelex ();
    yylval = prelval;
  }
  return (token);
}

static peeper (n)
int n;
{ register int i;
  if (peepb != 0)
  { for (i = peepb; i < peepe; i++)
      peep[i-peepb] = peep[i];
    peepe -= peepb;
  }
  peepb = 0;
  while (peepe < n)
  { peep[peepe].token = prelex ();
    peep[peepe].val = prelval;
    peepe++;
  }
}

/*
 *  prelex - return next token in date string
 *
 *     Uses nxtarg() to parse the next field of the date string.
 *  If a non-space, tab, comma or newline delimiter terminated the
 *  previous field it is returned before the next field is parsed.
 *
 *     Returns either one of the delimiter characters " -:/.", the token
 *  from a match on the table (with the associated value in yylval), or
 *  NUMBER, or JUNK.
 *  JUNK is any unrecognized token (depending on the call arguments to
 *  parsedate - an unrecognized token may instead be returned as -1 simply
 *  terminating the parse.)
 *  NUMBER is a numeric string.  NUMBER4 is a 4-digit number.
 *  If the numeric string commences with 0, then lead0 is true.
 *  '@' sign is treated as the token AT.
 */

static int wasabb;			/* tabfind indicates abbrev. */

static int prelex()
{

  register int ret;			/* temp for previous delimiter */
  register char *fp;			/* current field */
  register int find, ndig;
  extern char _argbreak;		/* current delimiter */

  while (1)
  { if (ret=delim)
    {
	delim = 0;
        if (ret == '@')
	    return (AT);
        /* 
         * Ignore all but the good characters
         */
	if (ret != ':' && ret != '/' && ret != '-' && ret != '.')
	    ret = 0;
        if (ret != 0)
	  return (ret);
    }
    if (*strp == 0) return (0);
    while (*strp == ' ' || *strp == '\t' || *strp == '\n')
      strp++;

    if (*strp >= '0' && *strp <= '9')
    { prelval = 0;
      ndig = 0;
      lead0 = *strp == '0';
      while (*strp >= '0' && *strp <= '9')
      { prelval = prelval * 10 + *strp - '0';
	strp++;
	ndig++;
      }
      if (ndig == 4)
        return (NUMBER4);
      return (NUMBER);
    }
    fp = nxtarg (&strp, " \t,-:/.@()[]\n");
    delim = _argbreak;
    if (*fp == 0 && delim == 0) return (0);  /* End of input string */
    if (*fp != 0) 			     /* skip null tokens */
    { foldup(fp, fp);
      /* Because of the embedded period, a.m. and p.m. do not work.
       * Solution is to recognize them explicitly.
       */
      if (fp[1] == '\0' && (delim == '.' || delim == ' ') &&
        (*fp == 'A' || *fp == 'P') && (*strp == 'M' || *strp == 'm') && 
        (strp[1] == '.' || strp[1] == ' ' || strp[1] == '\t' ||
          strp[1] == '\0'))
      { /* It is a.m. or p.m. */
        prelval = *fp == 'A' ? 0 : 12;
        strp += 2;    /* Skip the m. */
        delim = 0;
        return (AMPM);
      }
      if ((find = tabfind(fp, strings)) >= 0)
      {
	if (wasabb && delim == '.')	/* If tabfind found abbrev */
	  delim = 0;			/* Discard period after abbrev. */
        prelval = strings[find].lexval;
        return(strings[find].token);
      }
      if (junk_err)
        return (JUNK);
      else
	return (0);
    }
  }
}

/* subroutines useful for manipulating dates and used by
 * parsedate.y.
 * 
 * Copyright (c) Leonard G C Hamey, December 1983.
 * 
 * date_days (tm): converts the date portion of tm structure to days since
 *   1 jan 1900 + 693960. Also can be used to obtain weekday (sunday == 0)
 *   by taking modulo 7.
 *
 * days_date: converts days since 1/1/1900 + 693960 to date in tm structre.
 * 
 * tabfind: searches a table of keywords for date parsing.
 * 
 * constrain: fills in default fields under control of past/future
 * 	parameter.
 */

static int date_days (tm)
struct tm *tm;
{   /* Number of days since 1/1/1900 + 693960 */
    int dd = tm->tm_mday, mm = tm->tm_mon + 1, yyyy = tm->tm_year + 1900;
    int f;

    if (mm >= 3)
    {   f=365*yyyy+dd+31*mm-31-(4*mm+23)/10+yyyy/4-(75+(yyyy/100)*75)/100;
    }
    else
    {   f=365*yyyy+dd+31*mm-31+(yyyy-1)/4-(75+((yyyy-1)/100)*75)/100;
    }

    return (f-1);
}

static int monthend[] =
{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

static days_date (d, tm)
int d;
struct tm *tm;
{   /* Number of days since 1/1/1900 -> mm/dd/yyyy and weekday */
    int t = d - 693960, leap;
    int mm, yyyy;
    tm->tm_year = 0; 			     /* 1900 */
    tm->tm_mon = 0; 			     /* Jan */
    tm->tm_mday = 1;
    while (t < 0)			     /* handle dates before 1900 */
    {   tm->tm_year += t / 366 - 1;
        t = d - date_days (tm);
    }
    while (t >= 366)
    {   tm->tm_year += t / 366;
        t = d - date_days (tm);
    }
    yyyy = tm->tm_year + 1900;
    leap = yearsize (yyyy) == 366;
    if (! leap && t == 365)
    { tm->tm_year++;
      t = 0;
    }
    tm->tm_yday = t;			     /* Day in year (0-365) */
    if (! leap && t >= 59) 		     /* If after Feb non leap */
        t++; 				     /* Fudge as if leap */
    mm = t / 31; 			     /* Guess the month */
    if (t >= monthend[mm+1]) 		     /* Check the guess */
        mm++;
    tm->tm_mday = t - monthend[mm] + 1;	     /* Compute day in month */
    tm->tm_mon = mm;
    tm->tm_wday = d % 7; 		     /* Sunday = 0 */
}

static int tabfind (text, table)
char *text;
struct stable *table;
{ int tp;
  char *tep, *txp;
  int find = -1, foundstar;
  wasabb = 0;
  for (tp = 0; table[tp].text; tp++)
  { tep = table[tp].text;
    txp = text;
    foundstar = 0;
    while (1)
    { if (*tep == '*')
      { foundstar = 1;
	tep++;
      }
      if (! *txp) 			     /* If end of text */
      { if (! *tep) 			     /* If also end of table entry */
	  return (tp);			     /* then found */
	if (foundstar)
	{ if (find >= 0)
	    return (-2); 		     /* Ambiguous */
	  find = tp; 			     /* Remember partial match */
	  wasabb = 1;			     /* Was abbrev. */
	  break;
	}
      }
      if (*txp != *tep)
	break; 				     /* No match */
      tep++;  txp++;
    }
  }
  return (find);
}

/* check: check that a date is valid. each of the constraint processing
 *   routines is called in turn and if any of them do anything then the
 *   date is invalid.
 */

static check (date)
int date;
{ register int did;
  register struct dtm *d = &dtm[date];
  if (d->repn != RTM)
    return;
  did = month (d, d->tm.tm_mon, FUTURE);
  if (! did)
    did = mday (d, d->tm.tm_mday, FUTURE);
  if (! did && d->tm.tm_wday >= 0)
    did = weekday (d, d->tm.tm_wday, FUTURE);
  if (did)
    yyerror ();
  return;
}

/* constrain: fill in defaults info in date. con is a dtm containing the 
 *   constraints (or -1 indicating to use ptm). date is the dtm containing
 *   the date to be constrained. repeat is the loop count. */

static constrain (con, date, past, repeat)
int con, date;
int past, repeat;
{ register int n;
  register int did;
  register struct tm *c;
  register struct dtm *d = &dtm[date];
  if (con >= 0)
    c = &(dtm[con].tm);
  else
    c = &ptm;
  if (c->tm_year != NOYEAR)
    yyerror ();

  for (n = 1000, did = 0; ; did = 0)
  { if (c->tm_mon >= 0)
    { did |= month (d, c->tm_mon, past);
    }
    if (c->tm_mday >= 0)
    { did |= mday (d, c->tm_mday, past);
    }
    if (c->tm_wday >= 0)
    { did |= weekday (d, c->tm_wday, past);
    }
    if (! did)
    { if (repeat-- <= 1)
        break;
      incr (date, past ? -1 : 1);
    }
    if (--n <= 0)
      yyerror ();
  }

  if (con >= 0)
    dtmused[con] = 0;
  else
  { ptm.tm_year = NOYEAR;
    ptm.tm_mon = -1;
    ptm.tm_mday = -1;
    ptm.tm_wday = -1;
    ptm.tm_yday = -1;
  }
}

static time_def (tm, currtm)
struct tm *tm, *currtm;
{ if (shour >= 0)			/* Handle simple hour specification */
  { if (tmkey == -1)			/* and combine it with time key. */
      tmkey = 8;			/* Default is 8:00 - 19:59 */
    if (shour == 12)
      shour = 0;
    if (shour < tmkey)
      shour += 12;
    if (shour < tmkey)
      shour += 12;
    tm->tm_hour = shour;
  }
  if (tm->tm_hour >= 0)
  { /* If time specified and fields left out, assume zero. */
    if (tm->tm_min < 0)
      tm->tm_min = 0;
    if (tm->tm_sec < 0)
      tm->tm_sec = 0;
  }
}

/* date constraint processing routines.
 * 
 * These routines allow determination of the first date after/before
 * (but possibly equal to the existing date) which satisfies the given
 * constraint(s).
 */

/* The constraints are implemented by calling the appropriate routine(s)
 * which check whether the particular constraint is satisfied, and if it
 * is not, advances the date until the constraint is satisfied.
 * 
 * The date is stored in the dtm structure.
 */

/* weekday: constrains the day of the week. */

static int weekday (dtm, wkday, past)
struct dtm *dtm;
int wkday; 				     /* 0 = Sunday */
int past; 				     /* true = past */
{ int n;
  setrep (dtm, RDAYS);
  n = wkday - dtm->days % 7; 	     /* adjustment */
  if (past)
  { if (n > 0)
      n -= 7;
  }
  else {
    if (n < 0 || (nottoday && n == 0))
      n += 7;
    nottoday = 0;
  }
  dtm->days += n;
  return (n != 0);
}

/* month: constrains the month to the specified value. */

static int month (dtm, mon, past)
struct dtm *dtm;
int mon;
int past;
{ setrep (dtm, RTM);
  if (mon < 0 || mon > 11)
    yyerror ();
  if (dtm->tm.tm_mon != mon)
  { if (past)
    { if (dtm->tm.tm_mon < mon) 	     /* If earlier month */
	dtm->tm.tm_year--; 		     /* Back up a year */
      dtm->tm.tm_mday = monthend[mon+1] - monthend[mon];
      if (mon == 1 && yearsize (dtm->tm.tm_year+1900) == 365)  /* Feb */
	dtm->tm.tm_mday--;
    }
    else
    { if (dtm->tm.tm_mon > mon) 	     /* If later month */
	dtm->tm.tm_year++; 		     /* Advance a year */
      dtm->tm.tm_mday = 1;
    }
    dtm->tm.tm_mon = mon;
    dtm->tm.tm_wday = dtm->tm.tm_yday = -1;
    return (1);
  }
  return (0);
}

/* mday: constrains the month day to the specified value. Also
 *   checks the validity of the specified value and, if it is invalid,
 *   adjusts the month to compensate. */

static int mday (dtm, day, past)
struct dtm *dtm;
int day;
int past;
{ register int maxday;
  register int status = 0;
  setrep (dtm, RTM);
  if (dtm->tm.tm_mday != day)
  { if (past)
    { if (dtm->tm.tm_mday < day)  	     /* Earlier day */
        if (dtm->tm.tm_mon-- == 0)	     /* Back up a month */
	{ dtm->tm.tm_mon = 11;
	  dtm->tm.tm_year--;
	}
    }
    else
    { if (dtm->tm.tm_mday > day)  	     /* Later day */
	if (dtm->tm.tm_mon++ == 11)  	     /* Advance month */
	{ dtm->tm.tm_mon = 0;
	  dtm->tm.tm_year++;
	}
    }
    dtm->tm.tm_mday = day;
    dtm->tm.tm_wday = dtm->tm.tm_yday = -1;
    status = 1;
  }
  if (day >= 28)
  { maxday = monthend[dtm->tm.tm_mon+1] - monthend[dtm->tm.tm_mon];
    if (dtm->tm.tm_mon == 1 && yearsize (dtm->tm.tm_year + 1900) == 365)
      maxday--;
    if (day > maxday)
    { if (past)
        dtm->tm.tm_mday = maxday;
      else
      { dtm->tm.tm_mday = 1;
	if (dtm->tm.tm_mon++ == 11)
	{ dtm->tm.tm_mon = 0;
	  dtm->tm.tm_year++;
	}
      }
      dtm->tm.tm_wday = dtm->tm.tm_yday = -1;
      return (1);
    }
  }
  return (status);
}

/* setrep: sets the representation of the date in a dtm structure. */

static setrep (dtm, rep)
struct dtm *dtm;
int rep;
{ if (dtm->repn == rep)
    return;
  if (dtm->repn == RDAYS)
  { if (rep == RTM)
      days_date (dtm->days, &(dtm->tm));
  }
  else if (dtm->repn == RTM)
  { if (rep == RDAYS)
      dtm->days = date_days (&(dtm->tm));
  }
  dtm->repn = rep;
  return;
}

/* yearsize: returns nuber of days in year. */

static yearsize (year)
int year;
{ return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) ? 366 : 365);
}

/* new_dtm: returns the index of a new dtm structure. If current is CURRTM,
 *   the structure will contain a copy of the current date-time, else it
 *   will contain a copy of ptm, and ptm will be reset to all -1.
 */

static int new_dtm (current)
int current;
{ register int i;
  for (i = 0; i < NDTMS; i++)
    if (! dtmused[i])
    { dtmused[i] = 1;
      if (current == CURRTM || current == CURRDATE)
      { dtm[i].tm = *currtm;
        if (current == CURRDATE)
	{ dtm[i].tm.tm_hour = -1;
	  dtm[i].tm.tm_min = -1;
	  dtm[i].tm.tm_sec = -1;
	}
	dtm[i].repn = RTM;
	dtm[i].count = 0;
      }
      else
      { dtm[i].tm = ptm;
	dtm[i].count = pcount;
	dtm[i].repn = RTM;
	ptm.tm_year = NOYEAR;
	ptm.tm_mon = -1;
	ptm.tm_mday = -1;
	ptm.tm_wday = -1;
	ptm.tm_yday = -1;
	pcount = 0;
      }
      return (i);
    }
  yyerror ();
}

/* incr: increment date in dtm structure. */

static incr (ndtm, days)
int ndtm;
int days;
{ setrep (&dtm[ndtm], RDAYS);
  dtm[ndtm].days += days;
}

static incryear (ndtm, years)
int ndtm;
int years;
{ setrep (&dtm[ndtm], RTM);
  dtm[ndtm].tm.tm_year += years;
  dtm[ndtm].tm.tm_wday = -1;		/* Unknown */
  dtm[ndtm].tm.tm_yday = -1;		/* Unknown */
}

/* incrmonth: increment date in dtm structure by a number of months.*/

static incrmonth (ndtm, months)
int ndtm;
int months;
{ int inc;
  inc = months > 0 ? 1 : -1;
  setrep (&dtm[ndtm], RTM);		/* Use tm structure repn */
  for ( ; months != 0; months -= inc)
  { dtm[ndtm].tm.tm_mon += inc;
    if (dtm[ndtm].tm.tm_mon < 0)
    { dtm[ndtm].tm.tm_mon = 11;
      dtm[ndtm].tm.tm_year--;
    }
    else if (dtm[ndtm].tm.tm_mon > 11)
    { dtm[ndtm].tm.tm_mon = 0;
      dtm[ndtm].tm.tm_year++;
    }
  }
  dtm[ndtm].tm.tm_wday = -1; 		/* Day of week is unknown */
  dtm[ndtm].tm.tm_yday = -1;		/* Day of year is unknown */
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 21,
	285, 132,
	286, 132,
	-2, 137,
-1, 22,
	260, 127,
	282, 127,
	45, 127,
	47, 124,
	-2, 138,
-1, 34,
	274, 37,
	280, 37,
	-2, 39,
-1, 35,
	257, 33,
	258, 33,
	266, 33,
	267, 33,
	279, 33,
	45, 33,
	47, 33,
	-2, 40,
-1, 43,
	257, 32,
	258, 32,
	266, 32,
	267, 32,
	279, 32,
	45, 32,
	47, 32,
	-2, 41,
-1, 44,
	260, 128,
	282, 128,
	45, 128,
	-2, 43,
-1, 81,
	47, 124,
	-2, 127,
-1, 109,
	45, 128,
	-2, 43,
-1, 112,
	274, 35,
	-2, 20,
-1, 116,
	260, 128,
	282, 128,
	45, 128,
	-2, 44,
-1, 117,
	257, 32,
	258, 32,
	266, 32,
	267, 32,
	279, 32,
	45, 32,
	47, 32,
	-2, 45,
-1, 178,
	45, 128,
	-2, 44,
	};
# define YYNPROD 148
# define YYLAST 498
short yyact[]={

  22,  20,  36,  58,  32,  33,  30,   6,  41,  25,
  24,  50,  51,  52, 169, 166, 198, 197,  10,  49,
  12,  81,  26,  36,  58, 196, 168, 149,  19, 149,
  15,  15,  21,  57,  22, 106,  36,  58,  60,  61,
  30,  62,  41,  25,  24,  50,  51,  52, 101, 100,
 155,  15,  10,  49,  57,  68,  26,  75,  81,  60,
  61,  58,  62, 137,  15, 138,  71,  57,  22, 157,
  36,  58,  60,  61,  30,  62,  41,  25,  24,  50,
  51,  52, 102, 103, 104, 105,  10,  49, 156, 201,
  26,  57,  81,  72,  36,  58,  60,  61,  15,  62,
  71,  57,  22, 152,  36,  58,  60,  61,  30,  62,
  41,  25,  24,  50,  51,  52,  81, 163,  36,  58,
 174,  49,  57, 180,  26,  57,  81,  78,  36,  58,
  60,  61, 172,  62,  71,  57,  57,  78,  83, 164,
  60,  61, 146,  62,  93,  94,  15,  95,  83,  57,
  81,  97,  96,  58,  60,  61, 214,  62, 213,  57,
  91, 152, 113, 114,  60,  61, 167,  62, 119, 160,
 161, 162, 204,  92,  90,  20,  55,  20,  32,  33,
  32,  33, 182,  57, 221, 158, 183, 184,  60,  61,
  36,  62, 207, 150,  57, 204,  12, 219, 159, 122,
 123, 148,  19, 181,  19, 165,  21, 155,  67, 129,
 134, 129, 131, 129, 126,  99, 128,  15, 128, 135,
 128, 132,  36, 127, 139, 140,  45,  98,  64,   9,
  41, 146,  65, 143, 141,  18,  43,  86,  73, 152,
  76, 152, 144,  11,   8, 220, 142,  84,  40, 136,
  47,  86,  86,  86, 108,  46,  87, 147,  48,  77,
 124,  84,  84,  84, 117,   5,  79,  17, 125,  80,
  66, 150,  57, 150,  57, 139, 140,  16, 120,  27,
 110, 110, 110, 176, 143, 141, 121,  75,  34,  35,
  85,   7,   4,  74, 124, 108,   3, 142,  69,  82,
  31, 151,  39, 185,  85,  85,  85, 130, 133,  88,
  86,  38,  86, 107, 111, 112, 170, 118,  37, 173,
  84,  59, 179,  29, 145,  70,  13,  63, 115,  56,
 154,  44, 206,  86,  53,  54,  42,  28,  89, 110,
  23, 110, 151,  84,  14,   2,   1,   0,   0,   0,
   0, 202,   0,   0,   0,   0, 109, 109, 109, 116,
 175, 171, 110,  85,   0,   0,   0, 212,   0,   0,
   0, 189, 177, 190, 191, 187, 192, 153,  80, 193,
   0, 203, 205, 208, 209,   0,  85, 194, 195,   0,
   0,   0,   0,   0,   0, 186,   0,   0,   0,   0,
   0,  86, 215, 217,   0, 151,   0,   0, 210, 211,
 218,  84,   0,   0,   0, 109,   0, 178, 153,   0,
 216, 151,   0,  77, 199,   0,   0,   0,   0,   0,
  79,   0,   0,  80,   0,   0,   0,   0, 109,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 188,   0,   0,  85,   0,   0,   0,   0,   0,
   0,   0,   0,  82,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 200,
   0, 153,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 153 };
short yypact[]={

-257,-1000,-1000, -35, -81,-223,-1000,-188,-256,-225,
-141,-1000, -83,-124,-1000,-1000,-134, 169,-237,-1000,
-1000,-1000,-209,-1000,-236,-165,-165,-107,-1000,-1000,
 -70,-1000,-1000,-1000,-1000,-1000,-1000, -60, -62, -64,
-1000,-1000,  18,-1000, -37, -18,   5,-194,-213, -99,
-1000,-1000,-1000,-163,-135, 158,-1000,-1000,-284,-1000,
-102,-271,-285,-1000,-256,-256, -33,-1000,-189,-1000,
-124,-1000,-143,-1000,-155,-1000,-225,-1000,-1000,   5,
-1000,-209,-1000,-165,-1000,-199, -16,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-152,-1000,-1000, -75, -75,
-1000,-1000,-1000,-1000,-1000,-1000,-165,-1000,-1000,-1000,
 162,-1000,-1000,-1000,-1000,-1000,-126,-1000,-1000,-154,
-194,-213,-1000,-1000,-1000,-1000,-155,-1000,-155,-155,
-1000,-155,-1000,-1000,-155,-1000,-1000, -33, -33,-1000,
-1000,-272,-280,-281, -69,-1000,-1000,-168,-1000, -96,
-1000,-1000,-209,-1000, -88, -65, -88, -88,-1000,-1000,
-1000,-1000,-1000,-155,-155, -96,-110,-1000,-1000,-112,
-1000,-1000,-256,-1000,-131,-155, -88,-1000,-1000,-1000,
-256, 139,-1000,-1000,-1000,-1000,-1000,-225,-126,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-209,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, -73,
-1000,-1000 };
short yypgo[]={

   0, 346, 345, 296, 292, 265, 249, 323, 244, 229,
 283, 344, 340, 288, 279, 337, 300, 336, 236, 289,
 335, 334, 331, 329, 321, 268, 318, 311, 302, 248,
 325, 258, 250, 226, 255, 176, 201, 243, 291, 277,
 267, 203, 245, 235 };
short yyr1[]={

   0,   1,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   4,   4,   4,   4,   8,  11,  11,  11,  11,
  11,  11,  11,  11,  11,  11,  11,  11,  11,  16,
  16,  16,  17,  17,  20,  20,  20,  21,  21,  12,
  12,  13,  13,  13,  13,  13,  14,  23,  24,  24,
  24,  24,  24,  24,  24,  25,  25,  15,  15,  15,
  15,  15,  15,  15,  15,  15,  15,  15,  29,  29,
   7,   7,   7,   7,  27,  27,  28,  28,  26,  26,
  26,  26,  26,  26,  26,  26,  26,  26,  26,  19,
  19,  19,  19,  19,  18,  18,  18,  18,  18,  18,
   5,   5,   5,   3,   3,  38,   9,  37,  37,  37,
  37,  37,  37,  37,  37,  37,  39,  39,  33,   6,
   6,   6,   6,   6,  35,  36,  36,  34,  34,  40,
  40,  43,  43,  41,  41,  41,  42,  30,  30,  22,
  22,  22,  22,  22,  10,  10,  32,  31 };
short yyr2[]={

   0,   1,   1,   2,   3,   3,   2,   1,   1,   2,
   1,   1,   3,   4,   2,   1,   1,   3,   2,   2,
   2,   2,   2,   1,   1,   2,   2,   1,   2,   2,
   3,   3,   1,   1,   1,   2,   3,   1,   2,   1,
   1,   1,   1,   1,   2,   2,   1,   1,   1,   2,
   2,   2,   3,   2,   3,   2,   2,   2,   3,   2,
   2,   3,   2,   2,   3,   2,   1,   2,   3,   3,
   1,   1,   2,   2,   2,   2,   2,   2,   2,   1,
   1,   2,   2,   1,   2,   2,   1,   2,   1,   3,
   3,   4,   3,   3,   3,   2,   3,   3,   3,   1,
   1,   2,   2,   1,   4,   1,   1,   1,   4,   5,
   2,   2,   1,   2,   2,   1,   3,   3,   1,   1,
   1,   2,   2,   2,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   2,   2,   2,   1,   0,   2,   2 };
short yychk[]={

-1000,  -1,  -2,  -3,  -4,  -5, 264, -38,  -8,  -9,
 275, -37, 277, -30, -11, 287, -39, -40, -43, 285,
 258, 289, 257, -12, 267, 266, 279, -14, -15,  -7,
 263, -16, 261, 262, -13, -19, 259, -26, -27, -28,
 -29, 265, -17, -18, -22, -33, -34, -32, -31, 276,
 268, 269, 270, -21, -20, -35, -23, 290, 260, -24,
 295, 296, 298,  -7, 263, 267,  -5, 289, 278,  -4,
 -30, 289, 281,  -9, -10, 282,  -9, -29, 268, -34,
 -31, 257, -13, 279, -18, -14, -33, -37,  -3, -30,
 257, 284, 297, 268, 269, 281, 286, 285,  58,  46,
 286, 285, 291, 292, 293, 294, 271, -13,  -9, -22,
 -34, -13, -13, 269, 270, -16, -22, -18, -19, 275,
 -32, -31, 269, 270,  -9, -25, 274, 283, 280, 273,
 -25, 274, 283, -25, 274, 283,  -6,  45,  47, 257,
 258, 267, 279, 266, 279, -14, 268, 275, -36,  45,
 289, -34, 257, -22, -10,  45, 282, 282, 284, 297,
 268, 269, 270, 280, 274,  47, 299, 268, 297, 299,
  -6,  -4, 275,  -8, 275, -10, -10, -13, -22, -18,
 275, -41, 257, 261, 262, -41, -13, -34, -22,  -8,
  -8,  -8,  -8,  -8,  -6,  -6, 297, 297, 297, -14,
 -22, 257, -36, -33, 260, -33, -35, 257, -33, -33,
  -8,  -8, -36, 268, 268,  -9,  -8, -33,  -9,  58,
 -42, 257 };
short yydef[]={

   0,  -2,   1,   2,   7,   8,  10, 103,  11, 145,
   0, 100,   0, 105,  15, 106, 107, 131,   0, 112,
 115,  -2,  -2,  16,   0,   0,   0,  42,  23,  24,
  70,  27, 129, 130,  -2,  -2,  46,   0,   0,   0,
  66,  71,   0,  -2,  -2,   0, 145,  80,  79,   0,
  83,  86,  88,  34,   0,   0,  99, 139, 118,  47,
  48,   0,   0,   9,  70,   0,   3, 132,   0,   6,
   0, 137,   0,  14,   0, 144, 145,  67,  81, 145,
 146,  -2,  37,   0,  41,  42,   0, 101, 102, 105,
 138,  74,  76,  78,  84,   0, 111, 114,   0,   0,
 110, 113, 140, 141, 142, 143,   0,  18,  72,  -2,
   0,  19,  -2,  21,  22,  28,  -2,  -2,  33,   0,
   0,   0,  25,  26,  73,  57,   0,  59,   0,   0,
  60,   0,  62,  63,   0,  65,  29,   0,   0, 119,
 120,   0,   0,   0,   0,  38, 147,   0,  95,   0,
 125, 126, 127, 128,   0,   0,   0,   0,  75,  77,
  82,  85,  87,   0,   0,   0,  53,  49,  50,  51,
   4,   5,   0,  12,   0,   0,   0,  35,  -2,  45,
   0, 116, 133, 134, 135, 117,  17, 145, 128,  58,
  55,  56,  61,  64,  30,  31, 121, 122, 123,  36,
  89,   0,  96,  90, 118,  97,  98, 124,  92,  93,
  68,  69,  94,  54,  52, 104,  13,  91, 108,   0,
 109, 136 };
#ifndef lint
static	char yaccpar_sccsid[] = "@(#)yaccpar 1.1 85/12/19 SMI"; /* from UCB 4.1 83/02/11 */
#endif

#
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
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
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
			
case 1:
# line 254 "compat-cmucs/parsedate.y"
{ result = yyval;
			  check (yyval); } break;
case 2:
# line 259 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE); } break;
case 4:
# line 268 "compat-cmucs/parsedate.y"
{ setrep (&(dtm[yyval]), RTM);
			  dtm[yyval].tm.tm_year = ptm.tm_year;
			  dtm[yyval].tm.tm_wday = -1;
			  dtm[yyval].tm.tm_yday = -1; } break;
case 5:
# line 273 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 6:
# line 275 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 8:
# line 278 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE); } break;
case 9:
# line 280 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incr (yyval, yypvt[-0]); } break;
case 10:
# line 283 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  ptm.tm_hour = currtm->tm_hour;
			  ptm.tm_min = currtm->tm_min;
			  ptm.tm_sec = currtm->tm_sec; } break;
case 12:
# line 291 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 13:
# line 293 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 15:
# line 298 "compat-cmucs/parsedate.y"
{ check (yyval); } break;
case 16:
# line 302 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  constrain (PTM, yyval, ppf, 1); } break;
case 17:
# line 305 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  nottoday = 1;
			  constrain (PTM, yyval, FUTURE, 1); } break;
case 18:
# line 309 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  constrain (PTM, yyval, FUTURE, 1); } break;
case 19:
# line 312 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  constrain (PTM, yyval, FUTURE, 2); } break;
case 20:
# line 315 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incr (yyval, -1);
			  constrain (PTM, yyval, PAST, 1); } break;
case 21:
# line 319 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  constrain (PTM, yyval, FUTURE, 1);
			  incr (yyval, 7); } break;
case 22:
# line 323 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  constrain (PTM, yyval, FUTURE, 1);
			  incr (yyval, 14); } break;
case 24:
# line 328 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incr (yyval, yypvt[-0]); } break;
case 25:
# line 331 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incr (yyval, yypvt[-1] + 7); } break;
case 26:
# line 334 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incr (yyval, yypvt[-1] + 14); } break;
case 27:
# line 337 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM); } break;
case 28:
# line 339 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM); } break;
case 35:
# line 358 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM); } break;
case 36:
# line 360 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM);
			  dtm[yyval].count = yypvt[-2]; } break;
case 37:
# line 365 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM); } break;
case 38:
# line 367 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (PTM);
			  dtm[yyval].count = yypvt[-1]; } break;
case 43:
# line 382 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 44:
# line 384 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 46:
# line 389 "compat-cmucs/parsedate.y"
{ ptm.tm_wday = yypvt[-0]; } break;
case 47:
# line 393 "compat-cmucs/parsedate.y"
{ ptm.tm_mon = yypvt[-0]/100-1; ptm.tm_mday = yypvt[-0]%100; } break;
case 51:
# line 400 "compat-cmucs/parsedate.y"
{ yyval = 401; } break;
case 52:
# line 402 "compat-cmucs/parsedate.y"
{ yyval = 401; } break;
case 53:
# line 404 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] != 3)  yyerror ();
			  yyval = 401; } break;
case 54:
# line 407 "compat-cmucs/parsedate.y"
{ if (yypvt[-2] != 3)  yyerror ();
			  yyval = 401; } break;
case 55:
# line 416 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 56:
# line 418 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 57:
# line 426 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incr (yyval, yypvt[-1]); } break;
case 58:
# line 429 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incr (yyval, -yypvt[-2]); } break;
case 59:
# line 432 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRTM);
			  incr (yyval, -yypvt[-1]); } break;
case 60:
# line 435 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incrmonth (yyval, yypvt[-1]); } break;
case 61:
# line 438 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incrmonth (yyval, -yypvt[-2]); } break;
case 62:
# line 441 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incrmonth (yyval, -yypvt[-1]); } break;
case 63:
# line 444 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incryear (yyval, yypvt[-1]); } break;
case 64:
# line 447 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incryear (yyval, -yypvt[-2]); } break;
case 65:
# line 450 "compat-cmucs/parsedate.y"
{ yyval = new_dtm (CURRDATE);
			  incryear (yyval, -yypvt[-1]); } break;
case 67:
# line 454 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
case 68:
# line 463 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incr (yyval, 1);
			  constrain (yypvt[-2], yyval, FUTURE, dtm[yypvt[-2]].count); } break;
case 69:
# line 467 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0];
			  incr (yyval, -1);
			  constrain (yypvt[-2], yyval, PAST, dtm[yypvt[-2]].count); } break;
case 71:
# line 479 "compat-cmucs/parsedate.y"
{ tmkey = 17; } break;
case 73:
# line 482 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] == 0) yyerror (); } break;
case 74:
# line 490 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-1]; } break;
case 75:
# line 492 "compat-cmucs/parsedate.y"
{ yyval = 1; } break;
case 76:
# line 500 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-1]; } break;
case 77:
# line 502 "compat-cmucs/parsedate.y"
{ yyval = 1; } break;
case 81:
# line 513 "compat-cmucs/parsedate.y"
{ yyval = 1; } break;
case 82:
# line 515 "compat-cmucs/parsedate.y"
{ yyval = 1; } break;
case 83:
# line 517 "compat-cmucs/parsedate.y"
{ yyval = 1; } break;
case 84:
# line 519 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-1] * 7; } break;
case 85:
# line 521 "compat-cmucs/parsedate.y"
{ yyval = 7; } break;
case 86:
# line 523 "compat-cmucs/parsedate.y"
{ yyval = 7; } break;
case 87:
# line 525 "compat-cmucs/parsedate.y"
{ yyval = 14; } break;
case 88:
# line 527 "compat-cmucs/parsedate.y"
{ yyval = 14; } break;
case 89:
# line 536 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 92:
# line 540 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-2]; } break;
case 93:
# line 542 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-2]; } break;
case 105:
# line 571 "compat-cmucs/parsedate.y"
{ if (yypvt[-0] < 1 || yypvt[-0] > 12) yyerror ();
			  shour = yypvt[-0]; } break;
case 106:
# line 576 "compat-cmucs/parsedate.y"
{ tmkey = yypvt[-0]; } break;
case 110:
# line 584 "compat-cmucs/parsedate.y"
{ tmkey = yypvt[-0]; } break;
case 111:
# line 586 "compat-cmucs/parsedate.y"
{ tmkey = yypvt[-0]; } break;
case 112:
# line 588 "compat-cmucs/parsedate.y"
{ ptm.tm_hour = yypvt[-0]; } break;
case 113:
# line 590 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] != 12) yyerror ();
			  shour = -1;
			  ptm.tm_hour = yypvt[-0]; } break;
case 114:
# line 594 "compat-cmucs/parsedate.y"
{ if (shour != 12 || ptm.tm_min != 0) yyerror ();
			  shour = -1;
			  ptm.tm_hour = yypvt[-0]; } break;
case 115:
# line 598 "compat-cmucs/parsedate.y"
{ ptm.tm_hour = yypvt[-0] / 100;
			  ptm.tm_min = yypvt[-0] % 100;
			  if (ptm.tm_min > 59) yyerror ();
			  if (yypvt[-0] > 2400) yyerror (); } break;
case 118:
# line 609 "compat-cmucs/parsedate.y"
{ ptm.tm_mon = yypvt[-0]; } break;
case 119:
# line 618 "compat-cmucs/parsedate.y"
{ ptm.tm_year = yypvt[-0] + ((yypvt[-0]>=100)?-1900:0); } break;
case 120:
# line 620 "compat-cmucs/parsedate.y"
{ ptm.tm_year = yypvt[-0] - 1900; } break;
case 121:
# line 622 "compat-cmucs/parsedate.y"
{ ptm.tm_year = currtm->tm_year; } break;
case 122:
# line 624 "compat-cmucs/parsedate.y"
{ ptm.tm_year = currtm->tm_year - 1; } break;
case 123:
# line 626 "compat-cmucs/parsedate.y"
{ ptm.tm_year = currtm->tm_year + 1; } break;
case 124:
# line 634 "compat-cmucs/parsedate.y"
{ ptm.tm_mon = yypvt[-0] - 1; } break;
case 125:
# line 638 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 127:
# line 643 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 128:
# line 645 "compat-cmucs/parsedate.y"
{ ptm.tm_mday = yypvt[-0]; } break;
case 129:
# line 649 "compat-cmucs/parsedate.y"
{ ptm.tm_hour = yypvt[-0]; } break;
case 130:
# line 651 "compat-cmucs/parsedate.y"
{ shour = yypvt[-0]; } break;
case 132:
# line 656 "compat-cmucs/parsedate.y"
{ shour = yypvt[-0]; } break;
case 133:
# line 660 "compat-cmucs/parsedate.y"
{ ptm.tm_min = yypvt[-0]; } break;
case 134:
# line 662 "compat-cmucs/parsedate.y"
{ ptm.tm_min = yypvt[-0]; } break;
case 135:
# line 664 "compat-cmucs/parsedate.y"
{ ptm.tm_min = yypvt[-0]; } break;
case 136:
# line 668 "compat-cmucs/parsedate.y"
{ ptm.tm_sec = yypvt[-0]; } break;
case 140:
# line 682 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] % 10 != 1 || yypvt[-1] % 100 == 11) yyerror ();
			  yyval = yypvt[-1]; } break;
case 141:
# line 685 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] % 10 != 2 || yypvt[-1] % 100 == 12) yyerror ();
			  yyval = yypvt[-1]; } break;
case 142:
# line 688 "compat-cmucs/parsedate.y"
{ if (yypvt[-1] % 10 != 3 || yypvt[-1] % 100 == 13) yyerror ();
			  yyval = yypvt[-1]; } break;
case 143:
# line 691 "compat-cmucs/parsedate.y"
{ if ((yypvt[-1] + 9) % 10 <= 2 && (yypvt[-1] % 100) / 10 != 1)
			    yyerror ();
			  yyval = yypvt[-1]; } break;
case 146:
# line 701 "compat-cmucs/parsedate.y"
{ yyval = yypvt[-0]; } break;
		}
		goto yystack;  /* stack new state and value */

	}
