# include <sys/types.h>
# include <sys/time.h>

/* 
 *  HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX:  macro file name changed.
 *
 * 
 *  March 1984 - Leonard Hamey (lgh) at Carnegie-Mellon University
 *     Created.
 */

# define TEXT 1
# define FAIL 2
# define OPTIONAL 3
# define SUCCEED 4

# define YES 1
# define NO 0
# define MAYBE 2

# define SMONTHS 0
# define SWDAYS 12
# define SNOON 19
# define SMIDNIGHT 20
# define SAM 21
# define SPM 22
# define STH 23
# define SST 24
# define SND 25
# define SRD 26

# define NOON 0
# define HOUR 1
# define AM 2
# define MIN 3
# define SEC 4
# define WDAY 5
# define DAY 6
# define TH 7
# define NMONTH 8
# define MONTH 9
# define YEAR 10
# define YDAY 11
# define MIDNIGHT 12
# define TIME 13

static char *strings[] =
{ "jan*uary", "feb*ruary", "mar*ch", "apr*il", "may", "jun*e",
  "jul*y", "aug*ust", "sep*tember", "oct*ober", "nov*ember", "dec*ember",
  "sun*day", "mon*day", "tue*s*day", "wed*nesday", "thu*r*s*day",
  "fri*day", "sat*urday",
  "noon", "00:00", "am", "pm", "th", "st", "nd", "rd"
};

static char *keys[] =
{ "noon", "hour", "am", "min", "sec",
  "wday", "day", "th", "nmonth", "month", "year", "yday",
  "midnight", "time",
  0
};

/* The following is a macro definition of %time. */

static char *mactime =
  "[%midnight|%noon|%0hour:%min:%?sec|{%hour{:%?min}%am}]";

static char *p;
static int resp;
static char *result;
static dofield();
static percent();
static char foldc();
static formfld();
static copyword();
static casefix();

char *fdate (resb, patt, tm)
char *resb, *patt;
struct tm *tm;
{ result = resb;
  resp = 0;
  p = patt;
  dofield (' ', tm);
  result[resp] = '\0';
  return (result);
}

static dofield (doing, tm)
char doing;
struct tm *tm;
{ int ndep, s, sresp, sresp2;
  int fail;
  char ch;
  fail = MAYBE;
  sresp = resp;
  while (*p)
  { if (*p == '{' || *p == '[')
    { ch = *p++;
      s = dofield (ch, tm);
      if (s == FAIL && doing == '[')
	fail = YES;
      else if (s == SUCCEED && doing == '{')
	fail = NO;
    }
    else if (*p == '}' || *p == ']' || *p == '|')
    { if (doing == '{' && fail == MAYBE)
	fail = YES;
      if (fail == YES)
      { resp = sresp;
        if (*p == '|')
        { p++;
	  fail = MAYBE; 			     /* try next */
	  continue;
        }
	p++;
      }
      else if (*p == '|')
      { /* Scan to matching bracket/brace */
	ndep = 1;
	while (ndep > 0)
	{ if (*p == '{' || *p == '[')
	    ndep++;
	  else if (*p == '}' || *p == ']')
	    ndep--;
	  p++;
	}
      }
      else
	p++;
      return (fail == YES ? FAIL : SUCCEED);
    }
    else if (*p == '%')
    { sresp2 = resp;
      s = percent (tm);
      if (doing == '{')
      { if (s == FAIL)		     /* Discard failure results */
	  resp = sresp2;
	else if (s == SUCCEED)
	  fail = NO;
      }
      else if (doing == '[')
      { if (s == FAIL)
	  fail = YES;
      }
    }
    else
    { result[resp++] = *p;
      p++;
    }
    if (fail == YES)
    { /* Failure: scan for barline or closing bracket/brace; leave ptr there */
      resp = sresp;
      ndep = 1;
      while (ndep > 0)
      { if (*p == '|' && ndep == 1)
	  break;
	if (*p == '[' | *p == '{')
	  ndep++;
	else if (*p == ']' || *p == '}')
	{ ndep--;
	  if (ndep == 0)
	    return (FAIL);
	}
	p++;
      }
    }
  }
  return (SUCCEED);
}

static percent (tm)
struct tm *tm;
{ register char *pp;
  char *spp;
  int query, lead0, prec, upcase, sresp, s;
  char *savep;
  register int ks;
  register char *k;
  char foldc ();
  int formfld ();
  char *fldp;
  pp = p + 1;
  if (*pp == '%' || *pp == '{' || *pp == '}' || *pp == '|' ||
    *pp == '[' || *pp == ']')
  { p += 2;
    result[resp++] = *pp;
    return (TEXT); 			     /* Was just text */
  }
  query = *pp == '?'; 			     /* Query? */
  if (query)
    pp++;
  lead0 = *pp == '0';	 		     /* Leading zero? */
  if (lead0)
    pp++;
  if (*pp >= '1' && *pp <= '9') 	     /* Precision? */
  { prec = *pp - '0';
    pp++;
  }
  else
    prec = 0;
  upcase = 0;	 			     /* Case? */
  if (*pp >= 'A' && *pp <= 'Z')
    upcase = 1;
  if (pp[1] >= 'A' && pp[1] <= 'Z')
    upcase = 2;

  spp = pp;
  for (ks = 0; keys[ks]; ks++) 	     /* Check for keyword */
  { for (k = keys[ks], pp = spp; *k; k++, pp++)
      if (foldc (*k) != foldc (*pp))
        break;
    if (! *k)	 			     /* Match found */
      break;
  }
  if (! keys[ks]) 			     /* No match */
  { result[resp++] = '%'; 		     /* Treat as text */
    p++;
    return (TEXT);
  }
  p = pp;
  if (ks == TIME) 			     /* Macro */
  { savep = p;
    sresp = resp;
    p = mactime + 1; 			     /* Skip leading bracket */
    s = dofield (mactime[0], tm);
    p = savep;
    return (s);
  }
  /* Match found */
  s = formfld (tm, ks, lead0, prec, &fldp);
  sresp = resp;
  if (s == OPTIONAL)
  { if (query)
      s = FAIL;
    else
      s = SUCCEED;
  }
  if (s == SUCCEED)
  { for (; *fldp; fldp++)
      result[resp++] = *fldp;
    casefix (&result[sresp], upcase);
  }
  return (s);
}

static char foldc (c)
char c;
{ return (c < 'a' || c > 'z' ? c : c - 'a' + 'A');
}

static char nstr[30];
static char sfield[30];
static int thmem;

static formfld (tm, field, lead0, prec, result)
struct tm *tm;
int field, lead0, prec;
char **result;
{ int fld, ddiff, s;
  struct tm ctm, *localtime ();
  long ctime, time ();
  sfield[0] = '\0';
  *result = sfield;
  if (field != TH)
    thmem = -1;
  switch (field)
  { case NOON:
      if (tm->tm_hour == 12 && tm->tm_min == 0 && tm->tm_sec == 0)
      { *result = strings[SNOON];
	return (SUCCEED);
      }
      return (FAIL);

    case HOUR:
      for (; prec > 2; prec--)
	strcat (sfield, " ");
      if (lead0)
        sprintf (nstr, "%02d", tm->tm_hour);
      else if (prec < 2)
	sprintf (nstr, "%d", (tm->tm_hour + 11) % 12 + 1);
      else
	sprintf (nstr, "%2d", (tm->tm_hour + 11) % 12 + 1);
      strcat (sfield, nstr);
      thmem = tm->tm_hour;
      return (tm->tm_hour >= 0 ? SUCCEED : FAIL);

    case MIN:
      fld = tm->tm_min;
    case SEC:
      if (field == SEC)
	fld = tm->tm_sec;
      if (prec == 0)
	prec = 2;
      for (; prec > 2; prec--)
	strcat (sfield, " ");
      if (prec < 2)
	sprintf (nstr, "%d", fld);
      else
	sprintf (nstr, "%02d", fld);
      strcat (sfield, nstr);
      thmem = fld;
      return (fld > 0 ? SUCCEED : (fld == 0 ? OPTIONAL : FAIL));

    case AM:
      if (tm->tm_hour >= 12)
	*result = strings[SPM];
      else if (tm->tm_hour >= 0)
	*result = strings[SAM];
      return (tm->tm_hour >= 0 ? SUCCEED : FAIL);

    case WDAY:
      if (tm->tm_wday >= 0 && tm->tm_wday < 7)
      { copyword (sfield, strings[SWDAYS + tm->tm_wday], prec);
	return (SUCCEED);
      }
      return (FAIL);

    case MONTH:
      if (tm->tm_mon >= 0 && tm->tm_mon < 12)
      { copyword (sfield, strings[SMONTHS + tm->tm_mon], prec);
	return (SUCCEED);
      }
      return (FAIL);

    case DAY:
      fld = tm->tm_mday;
    case NMONTH:
    case YDAY:
      if (field == NMONTH)
	fld = tm->tm_mon + 1;
      else if (field == YDAY)
	fld = tm->tm_yday;
      if (prec == 0)
	prec = 1;
      if (lead0)
      { strcpy (nstr, "%09d");
	nstr[2] = prec + '0';
      }
      else
      { strcpy (nstr, "%9d");
	nstr[1] = prec + '0';
      }
      sprintf (sfield, nstr, fld);
      thmem = fld;
      return (fld >= 0 ? SUCCEED : FAIL);

    case TH:
      if (thmem >= 0)
      { if ((thmem / 10) % 10 == 1)
	  *result = strings[STH];
	else if (thmem % 10 == 1)
	  *result = strings[SST];
	else if (thmem % 10 == 2)
	  *result = strings[SND];
	else if (thmem % 10 == 3)
	  *result = strings[SRD];
	else
	  *result = strings[STH];
	return (SUCCEED);
      }
      return (FAIL);

    case YEAR:
      ctime = time (0);
      ctm = *localtime (&ctime);
      ddiff = (tm->tm_year - ctm.tm_year) * 366 + tm->tm_yday - ctm.tm_yday;
      if (tm->tm_year >= -1900)
      { if (prec == 0 || prec > 3)
	  sprintf (nstr, "%04d", tm->tm_year + 1900);
	else
	  sprintf (nstr, "%02d", (tm->tm_year + 1900) % 100);
	*result = nstr;
	return (ddiff < 0 || ddiff > 300 ? SUCCEED : OPTIONAL);
      }
      return (FAIL);

    case MIDNIGHT:
      if (tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0)
      { *result = strings[SMIDNIGHT];
	return (SUCCEED);
      }
      return (FAIL);
  }
}

/* copyword: Copy <from> to <to> subject to precision <prec>. Asterisks in
 * <from> mark valid truncation points. The longest string no longer than
 * <prec> and broken at a valid truncation point is returned in <to>. Note
 * that the result may exceed <prec> in length only if there are no valid
 * truncation points short enough. The shortest is then taken.
 * 
 * e.g. Tue*s*day.  Precision 1-3 -> Tue,  Precision 4-6 -> Tues,
 *      Precision 0 and 7... -> Tuesday.
 */

static copyword (to, from, prec)
char *to, *from;
int prec;
{ int ast = 0, top = 0;
  if (prec == 0)
    prec = 999;
  for (; *from; from++)
  { if (*from == '*')
      ast = top;
    else
    { to[top] = *from;
      top++;
    }
    if (top > prec && ast > 0)
    { /* Required precision exceeded and asterisk found */
      to[ast] = '\0'; 			     /* Truncate at asterisk */
      return;
    }
  }
  to[top] = '\0';
  return;
}

/* casefix: select case of word. 1: first letter raised. 2: all raised. */

static casefix (text, upcase)
char *text;
int upcase;
{ if (upcase == 1)
    *text = foldc (*text);
  else if (upcase > 0)
    foldup (text, text);
}
