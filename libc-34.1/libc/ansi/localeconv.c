/* Copyright (c) 1990 NeXT, Inc. - 3 August 90 John Anderson */

#include <c.h>
#include <libc.h>
#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <defaults.h>

#define NX_NUMBER_GROUP "\".\" \"\" 0"
#define NX_CURRENCY_GROUP "\"\" \"\" 0"
#define NX_NUMBER_FORMAT "\"\" \"\" \"\" \"\" 255 255 255 255 255 255 255 255"

static const NXDefaultsVector CurrencyDefaults = {		
    {"NXNumberGroup",		NX_NUMBER_GROUP},
    {"NXCurrencyGroup",		NX_CURRENCY_GROUP},
    {"NXCurrencyFormat",	NX_NUMBER_FORMAT},
    {NULL}
};

static const struct lconv C_lconv = {
	".", "", "", "", "", "", "", "", "", "",
	CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
	CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX
};

static struct lconv _lconv;

static char *
parseToken (char **sourcePtr, char **destinationPtr)
{
	bool		inQuote = FALSE;
	register char	theChar;
	register char	*theSource;
	char		*theToken;
	register char	*theDestination;

	theSource = *sourcePtr;
	theDestination = *destinationPtr;
	theToken = theDestination;
	theChar = *theSource;
	do {
	    if (theChar == '"') {
		inQuote = !inQuote;
	    } else {
		if (!inQuote) {
		    if (theChar == ' ') {
			do theChar = * (++theSource);
			while (theChar == ' ');
			break;
		    }
		    if (theChar == '\\')
			theChar = * (++theSource);
		}
		*theDestination++ = theChar;
		if (theChar == '\0')
		    break;
	    }
	    theChar = * (++theSource);
	} while (1);
	*theDestination++ = '\0';
	*sourcePtr = theSource;
	*destinationPtr = theDestination;
	return (theToken);
}


static char
parseNumber (char **sourcePtr, char **destinationPtr)
{
	register char	*theDestination;
	char		*theToken;
	int		theNumber;

	theDestination = parseToken (sourcePtr, destinationPtr);
	theToken = theDestination;
	theNumber = atoi (theToken);
	if (theNumber > CHAR_MAX)
	    theNumber = CHAR_MAX;
	*destinationPtr = theDestination;
	return ((char) theNumber);
}


static char *
parseGrouping (char **sourcePtr, register char **destinationPtr)
{
    register char	*theDestination;
    char		*theString;

    theDestination = *destinationPtr;
    theString = theDestination;
    while (**sourcePtr != '\0') {
	*theDestination++ = parseNumber (sourcePtr, destinationPtr);
	*destinationPtr = theDestination;
    }
    *theDestination++ = '\0';
    *destinationPtr = theDestination;
    return (theString);
}


#undef localeconv
struct lconv *localeconv(void)
{
    const char	*currencyFormat;
    const char	*currencyGroup;
    char	*destination;
    int		minimumSize;
    const char	*numberGroup;
    int		theSize;

    if (*(setlocale(LC_ALL, NULL)) != '\0')
	return ((struct lconv *)&C_lconv);
    else {
	if (!_lconv.decimal_point) {
	    NXRegisterDefaults ("GLOBAL", CurrencyDefaults);
	    numberGroup = NXGetDefaultValue ("GLOBAL",
	    				CurrencyDefaults[0].name);
	    currencyGroup = NXGetDefaultValue ("GLOBAL",
	    				CurrencyDefaults[1].name);
	    currencyFormat = NXGetDefaultValue ("GLOBAL",
	    				CurrencyDefaults[2].name);
	    theSize = strlen(numberGroup) + 1 +
	    	      strlen(currencyGroup) + 1 +
	    	      strlen(currencyFormat) + 1;
	    minimumSize = sizeof (NX_NUMBER_GROUP) +
	    		  sizeof (NX_CURRENCY_GROUP) +
	    		  sizeof (NX_NUMBER_FORMAT);
	    if (theSize < minimumSize)
		theSize = minimumSize;
	    destination = (char *) malloc (theSize);
	    _lconv.decimal_point =	parseToken (&numberGroup, &destination);
	    if (*_lconv.decimal_point == '\0')  /* Only decimal_point may not equal "" */
	        _lconv.decimal_point = ".";
	    _lconv.thousands_sep =	parseToken (&numberGroup, &destination);
	    _lconv.grouping =		parseGrouping (&numberGroup, &destination);

	    _lconv.int_curr_symbol =	parseToken (&currencyFormat, &destination);
	    _lconv.currency_symbol =	parseToken (&currencyFormat, &destination);

	    _lconv.mon_decimal_point =	parseToken (&currencyGroup, &destination);
	    _lconv.mon_thousands_sep =	parseToken (&currencyGroup, &destination);
	    _lconv.mon_grouping =	parseGrouping (&currencyGroup, &destination);

	    _lconv.positive_sign =	parseToken (&currencyFormat, &destination);
	    _lconv.negative_sign =	parseToken (&currencyFormat, &destination);

	    _lconv.int_frac_digits =	parseNumber (&currencyFormat, &destination);
	    _lconv.frac_digits =	parseNumber (&currencyFormat, &destination);
	    _lconv.p_cs_precedes =	parseNumber (&currencyFormat, &destination);
	    _lconv.p_sep_by_space =	parseNumber (&currencyFormat, &destination);
	    _lconv.n_cs_precedes =	parseNumber (&currencyFormat, &destination);
	    _lconv.n_sep_by_space =	parseNumber (&currencyFormat, &destination);
	    _lconv.p_sign_posn =	parseNumber (&currencyFormat, &destination);
	    _lconv.n_sign_posn =	parseNumber (&currencyFormat, &destination);
	}
	return (&_lconv);
    }
}
