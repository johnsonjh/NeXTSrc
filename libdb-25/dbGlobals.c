#pragma CC_NO_MACH_TEXT_SECTIONS
#ifdef SHLIB
#include "shlib.h"
#endif

/* 
 * these are the global variables that are used in the libdb library.
 * all global data MUST reside here.  Also, if there is a change in the
 * size of the data it must be put exactly before the appropriate pad
 * and that pad should change its size to keep the same total data size. 

 * the following output should never change except for the ..._pad variables.
 * they will move down as other data is placed at the end of each section.
 * 	nm -n dbGlobals.o > 
	# __text section
		00000000 t __db_global_text_pad	# start of global const data
		00000100 t __dbDirExtension	# start of literatl const data
		00000103 t __dbLeafExtension
		00000106 t __db_literal_text_pad
	# __data section
		00000200 D __dbLeafCount	# start of global data
		00000204 D __dbstrbuf
		000002cc D _dbMode
		000002d0 D _dbFlags
		000002d4 D __DB
		000002d8 D _dbErrors
		000002dc D _dbMaxLeafBuf
		000002e0 D _dbLeafSize
		000002e4 D _dbMsgs
		000002e8 D _dbDirExtension
		000002ec D _dbLeafExtension
		000002f0 D _dbErrorNo
		000002f4 D __db_data_pad
 */
 
#ifdef SHLIB
#include "shlib.h"
#endif
#include "internal.h"

/********************************
 *   __text section stuff	*
 * 				*
 ********************************/
/* global const data would go here. */	
/* it would look like static const data below w/o the static */
static const char _db_global_text_pad[256] = {0};

/* static (literal) const data goes here */
static const char _dbDirExtension[] = ".D"; /* default dir file extension */
static const char _dbLeafExtension[]= ".L"; /* default leaf file extension */
static const char _db_literal_text_pad[248] = {0};


/********************************
 *   __data section stuff	*
 * 				*
 ********************************/
/* private to libdb */
int _dbLeafCount = 0;		/* not static because dbClose uses it */
char _dbstrbuf[BSIZE] = { 0 };
Database *_DB = (Database *)0;	/* not used previously -- commandeered for 
				chain of open dbs for buffer sharing */

/* global to libdb */
int dbErrors = 0;		/* if true enable error messages */
int dbFlags = -1;		/* dbOpen flags */
int dbMode = 0644;		/* dbOpen mode */
int dbMaxLeafBuf = _MaxLeafBuf;	/* max number of lbufs (+-1)  per file */
				/* 1000 ~= 1 meg */
int dbLeafSize = LEAFSIZE;	/* Default Leaf block size */

FILE *dbMsgs = stderr;		/* where to write diagnostics */

/* these are needed to put the literals
 * in the text segment properly for shlibs
 */
char *dbDirExtension = (char *) _dbDirExtension;
char *dbLeafExtension= (char *) _dbLeafExtension;

/* global to libdb (continued) */
dbErrorType dbErrorNo = dbErrorNoError;	/* contains last error code reported */
unsigned int *dbSizeTable = 0;

char _db_data_pad[264] = {0};
