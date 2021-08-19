/* Copyright (c) 1990 NeXT, Inc. - 3 August 90 John Anderson */

#include <c.h>
#include <locale.h>
#include <stddef.h>
#include <time.h>
#include "strftimeprivate.h"
#include <defaults.h>

#define SHORT_DAYS	0	   /* Index into the DateTimeDefaults array */
#define LONG_DAYS	1
#define SHORT_MONTHS	2
#define LONG_MONTHS	3
#define AM_PM		4
#define DATE_AND_TIME	5
#define DATE		6
#define TIME		7
#define NUM_DAYS	7
#define NUM_MONTHS	12
#define NUM_AM_PM	2


static const NXDefaultsVector DateTimeDefaults = {		
    {"NXShortDays",	"Sun Mon Tue Wed Thu Fri Sat"},		/* Abbreviated day names */
    {"NXLongDays",	"Sunday Monday Tuesday Wednesday"	/* Long day names */
			" Thursday Friday Saturday"},
    {"NXShortMonths",	"Jan Feb Mar Apr May Jun Jul Aug"	/* Abbreviated month names */
			" Sep Oct Nov Dec"},
    {"NXLongMonths", 	"January February March April May"	/* Long month names */
			" June July August September October"
			" November December" },
    {"NXAmPm",		"AM PM"},				/* AM PM designation */
    {"NXDateAndTime",	"%a %b %d %H:%M:%S %Z %Y"},		/* default date and time representation */
    {"NXDate",		"%a %b %d %Y"},				/* default date representation */
    {"NXTime",		"%H:%M:%S %Z"},				/* default time representation */
    {NULL}
};

static char *_ShortDays[NUM_DAYS] = { 0 };
static char *_LongDays[NUM_DAYS] = { 0 };
static char *_ShortMonths[NUM_MONTHS] = { 0 };
static char *_LongMonths[NUM_MONTHS] = { 0 };
static char *_AmPm[NUM_AM_PM] = { 0 };
static char *_DateAndTime[1] = { 0 };
static char *_Date[1] = { 0 };
static char *_Time[1] = { 0 };

static bool RegisteredDefaults = FALSE;

void _strftime_clear_cache(void)
{
	memset (_ShortDays, 0, sizeof (_ShortDays));
	memset (_LongDays, 0, sizeof (_LongDays));
	memset (_ShortMonths, 0, sizeof (_ShortMonths));
	memset (_LongMonths, 0, sizeof (_LongMonths));
	memset (_AmPm, 0, sizeof (_AmPm));
	_DateAndTime[0] = NULL;
	_Date[0] = NULL;
	_Time[0] = NULL;
}

static const char *
LookupString (const char *stringPtrArray[], const int indexInName, const int arrayLimit, const int indexOfName)
{
    int		index;
    char	theChar;
    const char	*theString;
    const char	**theStringPtrArray;

    if (*stringPtrArray)
	return (*(stringPtrArray + indexInName));
    else {
	if (*(setlocale(LC_ALL, NULL)) == '\0') {
	    if (!RegisteredDefaults) {
		NXRegisterDefaults ("GLOBAL", DateTimeDefaults);
		RegisteredDefaults = TRUE;
	    }
	    theString = NXGetDefaultValue ("GLOBAL",
	    				DateTimeDefaults[indexOfName].name);
	}
	else
	    theString = DateTimeDefaults[indexOfName].value;
     /*
      * If the arrayLimit is 1 (i.e. there is only one string, we'll assume
      * that substrings are not separated by spaces.
      */
	if (arrayLimit == 1)
	    *stringPtrArray = theString;
	else {
	    theStringPtrArray = stringPtrArray;
	    index = 0;
	    theChar = *theString;
	    do
		if (theChar) {
		    *theStringPtrArray++ = theString;
		    do {
			if (theChar == ' ') {
			    do theChar = * (++theString);
			    while (theChar == ' ');
			    break;
			}
			if (theChar == '\\')
			    theChar = * (++theString);
			if (theChar == '\0')
			    break;
			theChar = * (++theString);
		    } while (TRUE);
		} else
		    *theStringPtrArray++ = NULL;
	    while (++index < arrayLimit);
	}
	return (*(stringPtrArray + indexInName));
    }
}

#undef strftime
size_t
strftime(char *s, size_t maxsize, const char *format, const struct tm *tp)
{
    size_t	charCount = 0;
    size_t	charsProcessed;
    char	localString[40];
    bool	recurse;
    char	theChar;
    const char	*subString;
    char	terminateChar;

    while ((theChar = *format++) != '\0') {
        if (theChar != '%') {
	    if (++charCount > maxsize) return 0;
	    *s++ = theChar;
	} else {
            subString = localString;
	    recurse = FALSE;
	    switch (theChar = *format++) {
		case '\0': format--;						    /* fall through */
		case '%': subString = "%";
			  break;
		case 'Z': subString = tp->tm_zone;
			  break;
		case 'd': sprintf(localString, "%02d", tp->tm_mday);
			  break;
		case 'H': sprintf(localString, "%02d", tp->tm_hour);
			  break;
		case 'I': sprintf(localString, "%02d", ((tp->tm_hour+11)%12)+1);
			  break;
		case 'j': sprintf(localString, "%03d", tp->tm_yday);
			  break;
		case 'm': sprintf(localString, "%02d", tp->tm_mon+1);
			  break;
		case 'M': sprintf(localString, "%02d", tp->tm_min);
			  break;
		case 'S': sprintf(localString, "%02d", tp->tm_sec);
			  break;
		case 'U': sprintf(localString, "%02d", (tp->tm_yday+7-tp->tm_wday)/7);
			  break;
		case 'w': sprintf(localString, "%01d", tp->tm_wday);
			  break;
		case 'W': sprintf(localString, "%02d", (tp->tm_yday+7-((tp->tm_wday+6)%7))/7);
			  break;
		case 'y': sprintf(localString, "%02d", tp->tm_year % 100);
			  break;
		case 'Y': sprintf(localString, "%04d", tp->tm_year + 1900);
			  break;
		case 'a': subString = LookupString (_ShortDays, tp->tm_wday, NUM_DAYS, SHORT_DAYS);
			  break;
		case 'A': subString = LookupString (_LongDays, tp->tm_wday, NUM_DAYS, LONG_DAYS);
			  break;
		case 'b': subString = LookupString (_ShortMonths, tp->tm_mon, NUM_MONTHS, SHORT_MONTHS);
			  break;
		case 'B': subString = LookupString (_LongMonths, tp->tm_mon, NUM_MONTHS, LONG_MONTHS);
			  break;
		case 'p': subString = LookupString (_AmPm, tp->tm_hour>=12, NUM_AM_PM, AM_PM);
			  break;
		case 'c': subString = LookupString (_DateAndTime, 0, 1, DATE_AND_TIME);
			  recurse = TRUE;
			  break;
		case 'x': subString = LookupString (_Date, 0, 1, DATE);
			  recurse = TRUE;
			  break;
		case 'X': subString = LookupString (_Time, 0, 1, TIME);
			  recurse = TRUE;
			  break;
		default: subString = "?";
	    }
	    if (subString)
		if (recurse) {
		 /* We won't worry about infinite recursion here */
		    charsProcessed = strftime (s, maxsize - charCount, subString, tp);
		    if (charsProcessed == 0) return 0;
		    charCount += charsProcessed;
		    s += charsProcessed;
		} else {
		 /*
		  * We store all the components of months, days, etc. as substrings separated
		  * by spaces in one large string.  Therefore we need to terminate on space and
		  * '\0' with all the strings returned by LookupString that we don't recurse on.
		  * We can easily detect this case since subString != localString.  When
		  * terminateChar == '\0' we have an unnecessary extra test in the loop.  You
		  * can escape spaces with backslash but you can't escape '\0' with backslash.
		  */
		    terminateChar = '\0';
		    if (subString != localString)
			terminateChar = ' ';
		    do {
			theChar = *subString++;
			if (theChar == terminateChar)
			    break;
			if (theChar == '\\')
			    theChar = *subString++;
			if (theChar == '\0')
			    break;
			if (++charCount > maxsize) return 0;
			*s++ = theChar;
		    } while (TRUE);
		}
	}
    }
    if (++charCount > maxsize) return 0;
    *s = '\0';
    return (charCount - 1);
}
