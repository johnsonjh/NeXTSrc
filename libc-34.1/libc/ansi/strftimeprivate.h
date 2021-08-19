/* Copyright (c) 1990 NeXT, Inc. - 3 August 90 John Anderson */

/* declare string caches used in strftime which are cleared by setlocale */

#define NUM_DAYS		7
#define NUM_MONTHS		12
#define NUM_AM_PM		2

extern const char *_ShortDays[NUM_DAYS];
extern const char *_LongDays[NUM_DAYS];
extern const char *_ShortMonths[NUM_MONTHS];
extern const char *_LongMonths[NUM_MONTHS];
extern const char *_AmPm[NUM_AM_PM];
extern const char *_DateAndTime[1];
extern const char *_Date[1];
extern const char *_Time[1];
