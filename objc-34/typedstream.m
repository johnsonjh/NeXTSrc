/*
    typedstream.m
    Pickling data structures
    Copyright 1989, NeXT, Inc.
    Responsability: Bertrand Serlet
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>
#import <stdarg.h>
#import <mach.h>
#import <sys/file.h>
#import "objc-private.h"
#import "Object.h"
#import "objc-class.h"
#import "objc-runtime.h"
#import "typedstream.h"
#import "typedstreamprivate.h"
#import "hashtable.h"

/*************************************************************************
 *	Utilities
 *************************************************************************/

static void checkRead (TypedStream *s) {
    if (s->write) 
	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "expecting a reading stream", s);
    };
    
static void checkWrite (TypedStream *s) {
    if (! s->write) 
	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "expecting a writing stream", s);
    };

static void checkExpected (const char *readType, const char *wanted) {
    if (readType == wanted) return;
    if (! readType || strcmp (readType, wanted)) {
	char	*buffer = malloc (
		((readType) ? strlen (readType) : 0) + strlen (wanted) + 100);
	sprintf (buffer, "file inconsistency: read '%s', expecting '%s'", 
	    readType, wanted);
	NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, buffer, 0);
	};
    };
    
static void classError (const char *className, const char *message) {
    char	*buffer = malloc (100);
    sprintf (buffer, "class error for '%s': %s", className, message);
    NX_RAISE (TYPEDSTREAM_CLASS_ERROR, buffer, 0);
    };
    
static void typeDescriptorError (char ch, const char *message) {
    char	*buffer = malloc (100);
    sprintf (buffer, "type descriptor error for '%c': %s", ch, message);
    NX_RAISE (TYPEDSTREAM_TYPE_DESCRIPTOR_ERROR, buffer, 0);
    };
    
static void writeRefError (const char *message) {
    NX_RAISE (TYPEDSTREAM_WRITE_REFERENCE_ERROR, (char *) message, 0);
    };
    
static char *globalBuffer = NULL;
static int globalBufferSize = 0;

static char *buffer (int length) {
    if (length+1 > globalBufferSize) {
	globalBufferSize = length+1;
	globalBuffer = (char *) realloc (globalBuffer, globalBufferSize);
	};
    return globalBuffer;
    };

static BOOL sameSexAsCube () {
    union {int	ii; char cc[sizeof(int)];}	uu;
    
    uu.ii = 1;
    if (uu.cc[sizeof(int)-1] == 1) return TRUE;
    if (uu.cc[0] == 1) return FALSE;
    NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, 
    	"typedstream: snail? cannot recognize byte sex.", 
	(void *) (int) uu.cc[0]);
    };
    
/*************************************************************************
 *	Low-Level: encoding tightly information, 
 *	and sharing pointers and strings
 *************************************************************************/

#define LONG2LABEL	-127
#define LONG4LABEL	-126
#define REALLABEL	-125
#define NEWLABEL	-124
#define NULLLABEL	-123
#define EOOLABEL	-122
/* more reserved labels */
#define SMALLESTLABEL	-110
#define Bias(x) (x - SMALLESTLABEL)

/* Following constants are not universal constants, but used for the coding */
#define BIG_INT +2147483647L
#define SMALL_INT -2147483647L
#define BIG_SHORT +32767
#define SMALL_SHORT -32767

#define INIT_NSTRINGS 252
#define INIT_NPTRS 252
#define PTRS(x) ((void **)x)
#define STRINGS(x) ((char **)x)

static inline int inc_stringCounter(_CodingStream *coder) {
  if (coder->stringCounter == coder->stringCounterMax) {
    coder->stringCounterMax += INIT_NSTRINGS;
    coder->strings = (NXHashTable *)
	NXZoneRealloc(coder->scratch, coder->strings, coder->stringCounterMax * sizeof(char *));
  }
  return coder->stringCounter++;
}

static int inc_ptrCounter(_CodingStream *coder) {
  if (coder->ptrCounter == coder->ptrCounterMax) {
    coder->ptrCounterMax += INIT_NPTRS;
    coder->ptrs = (NXHashTable *)
	NXZoneRealloc(coder->scratch, coder->ptrs, coder->ptrCounterMax * sizeof(void *));
  }
  return coder->ptrCounter++;
}

typedef struct {
    const char	*key;
    int		value;
    } StrToInt;

typedef struct {
    const void	*key;
    int		value;
    } PtrToInt;

typedef struct {
    int		key;
    void	*value;
    } IntToPtr;

   
/* Creation, destruction of the low-level streams */
static _CodingStream *_NXOpenEncodingStream (NXStream *physical) {
    _CodingStream		*coder;
    
    coder = (_CodingStream *) malloc (sizeof (_CodingStream));
    coder->physical = physical;
    coder->swap = NO;
    coder->write = YES;
    coder->strings = NXCreateHashTable (NXStrStructKeyPrototype, 7, NULL);
    coder->stringCounter = SMALLESTLABEL;
    coder->ptrs = NXCreateHashTable (NXPtrStructKeyPrototype, 15, NULL);
    coder->ptrCounter = SMALLESTLABEL;
    return coder;
    };

static _CodingStream *_NXOpenDecodingStream (NXStream *physical) {
    _CodingStream		*coder;
    NXZone *zone;
    
    zone = NXCreateZone(vm_page_size, vm_page_size, 1);
    coder = (_CodingStream *) NXZoneMalloc (zone, sizeof (_CodingStream));
    coder->scratch = zone;
    coder->physical = physical;
    coder->swap = NO;
    coder->write = NO;
    coder->strings = (NXHashTable *)
	NXZoneMalloc(coder->scratch, INIT_NSTRINGS * sizeof(char *));
    coder->stringCounter = 0;
    coder->stringCounterMax = INIT_NSTRINGS;
    coder->ptrs = (NXHashTable *)
	NXZoneMalloc(coder->scratch, INIT_NPTRS * sizeof(void *));
    coder->ptrCounter = 0;
    coder->ptrCounterMax = INIT_NPTRS;
    return coder;
    };

static BOOL _NXEndOfCodingStream (_CodingStream *coder) {
    char	ch = NXGetc (coder->physical);
    NXUngetc (coder->physical);
    return ch == -1;
    };

static void _NXCloseCodingStream (_CodingStream *coder) {
  if (coder->write) {
	NXFreeHashTable(coder->strings);
	NXFreeHashTable(coder->ptrs);
	free(coder);
    } else 
	NXDestroyZone(coder->scratch);
  };

/* Encoding/Decoding of usual quantities */

static void _NXEncodeBytes (_CodingStream *coder, const char *buf, int count) {
    if (coder->physical) NXWrite (coder->physical, buf, count);
    };
    
static void _NXDecodeBytes (_CodingStream *coder, char *buf, int count, BOOL swap) {
    NXRead (coder->physical, buf, count);
    if (swap) {
	int	index = count >> 1; /* we swap all bytes */
	while (index--) {
	    char	ch = buf[index];
	    buf[index] = buf[count-index-1];
	    buf[count-index-1] = ch;
	    };
	};
    };

static void _NXEncodeChar (_CodingStream *coder, signed char c) {
    if (coder->physical) NXPutc (coder->physical, c);
    };
    
static signed char _NXDecodeChar (_CodingStream *coder) {
    return NXGetc (coder->physical);
    };

/* all the following should (of course) be made machine independent */
static void _NXEncodeShort (_CodingStream *coder, short x) {
    if (x>=SMALLESTLABEL && x<=127)
	_NXEncodeChar (coder, (signed char) x);
    else {
	_NXEncodeChar (coder, LONG2LABEL); 
	_NXEncodeBytes (coder, (char *) &x, 2);
	};
    };
    
static short _NXDecodeShort (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    short	x;
    if (ch != LONG2LABEL) return (short) ch;
    _NXDecodeBytes (coder, (char *) &x, 2, coder->swap);
    return x;
    };
	
static void _NXEncodeInt (_CodingStream *coder, int x) {
    if (x >= SMALLESTLABEL && x <= 127)
	_NXEncodeChar (coder, (signed char) x);
    else if (x >= SMALL_SHORT && x <= BIG_SHORT) {
	short	sh = (short) x;
	_NXEncodeChar (coder, LONG2LABEL);
	_NXEncodeBytes (coder, (char *) &sh, 2);
	}
    else {
	_NXEncodeChar (coder, LONG4LABEL);
	_NXEncodeBytes (coder, (char *) &x, 4);
	};
    };

/* Finishes to decode an int. ch can be only be LONG2LABEL, LONG4LABEL or the int itself. */
static int FinishDecodeInt (_CodingStream *coder, signed char ch) {
    switch (ch) {
    	case LONG2LABEL: {
	    short	x;
	    _NXDecodeBytes (coder, (char *) &x, 2, coder->swap);
	    return (int) x;
	    };
	case LONG4LABEL: {
	    int		x;
	    _NXDecodeBytes (coder, (char *) &x, 4, coder->swap);
	    return x;
	    };
	default: return (int) ch;
	};
    };
    
static int _NXDecodeInt (_CodingStream *coder) {
    return FinishDecodeInt (coder, _NXDecodeChar (coder));
    };
	
static int FloorOrZero (double x) {
    if (x >= (double) BIG_INT-1) return 0;
    if (x < (double) SMALL_INT+1) return 0;
    return (int) (float) x;
    };
    
static void _NXEncodeFloat (_CodingStream *coder, float x) {
    int		flore = FloorOrZero ((double) x);
    /* we are conservative */
    if ((float) flore == x) _NXEncodeInt (coder, flore); 
    else {
	_NXEncodeChar (coder, REALLABEL);
        _NXEncodeBytes (coder, (char *) &x, sizeof(float));
	};
    };

static float _NXDecodeFloat (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    float	x;
    if (ch != REALLABEL) return (float) FinishDecodeInt (coder, ch);
    _NXDecodeBytes (coder, (char *) &x, sizeof(float), coder->swap);
    return x;
    };
	
static void _NXEncodeDouble (_CodingStream *coder, double x) {
    int		flore = FloorOrZero (x);
    if ((double) flore == x) _NXEncodeInt (coder, flore);
    else {
	_NXEncodeChar (coder, REALLABEL);
        _NXEncodeBytes (coder, (char *) &x, sizeof(double));
	};
    };

static double _NXDecodeDouble (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    double	x;
    if (ch != REALLABEL) return (double) FinishDecodeInt (coder, ch);
    _NXDecodeBytes (coder, (char *) &x, sizeof(double), coder->swap);
    return x;
    };
	
static void _NXEncodeChars (_CodingStream *coder, const char *str) {
    int		len;
    if (! str) {_NXEncodeChar (coder, NULLLABEL); return;};
    len = strlen (str);
    _NXEncodeInt (coder, len);
    _NXEncodeBytes (coder, str, len);
    };

static char *_NXDecodeChars (_CodingStream *coder, NXZone *zone) {
    signed char	ch = _NXDecodeChar (coder);
    int		len;
    STR		str;
    if (ch == NULLLABEL) return NULL;
    len = FinishDecodeInt (coder, ch);
    str = (STR) NXZoneMalloc (zone, len+1);
    _NXDecodeBytes (coder, str, len, FALSE);
    str[len] = '\0';
    return str;
    };
    	
/* Encoding/Decoding of shared quantities.  The trick is that a label int cannot start by encoding NEWLABEL */
static void _NXEncodeSharedString (_CodingStream *coder, const char *str) {
    StrToInt	sti;
    
    if (! str) {_NXEncodeChar (coder, NULLLABEL); return;};
    sti.key = str;
    if (NXHashMember (coder->strings, &sti)) {
	int	value = ((StrToInt *) NXHashGet (coder->strings, &sti))->value;
	_NXEncodeInt (coder, value);
	return;
	}
     else {
	 StrToInt	*new = (StrToInt *) malloc (sizeof (StrToInt));
    	_NXEncodeChar (coder, NEWLABEL); _NXEncodeChars (coder, str);
	new->key = str; new->value = coder->stringCounter++;
	(void) NXHashInsert (coder->strings, new);
	return;
	};
    };
    
static char *_NXDecodeSharedString (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    
    if (ch == NULLLABEL) return NULL;
    if (ch == NEWLABEL) {
	char *s = _NXDecodeChars (coder, coder->scratch);
	STRINGS(coder->strings)[inc_stringCounter(coder)] = s;
	return s;
	};
    return STRINGS(coder->strings)[Bias(FinishDecodeInt(coder, ch))];
    };

static const char *_NXDecodeUniqueString (_CodingStream *coder) {
    return NXUniqueString (_NXDecodeSharedString (coder));
    };
    
static BOOL _NXEncodeShared (_CodingStream *coder, const void *ptr) {
    PtrToInt	pti;
    
    if (! ptr) {
	_NXEncodeChar (coder, NULLLABEL);
	return NO;
	};
    pti.key = ptr;
    if (NXHashMember (coder->ptrs, &pti)) {
	int	value = ((PtrToInt *) NXHashGet (coder->ptrs, &pti))->value;
	_NXEncodeInt (coder, value);
	return NO;
	}
     else {
	 PtrToInt	*new = (PtrToInt *) malloc (sizeof (PtrToInt));
	_NXEncodeChar (coder, NEWLABEL);
	new->key = ptr; new->value = coder->ptrCounter++;
	(void) NXHashInsert (coder->ptrs, new);
	return YES;
	};
    };
    
/*
static BOOL _NXDecodeShared (_CodingStream *coder, void **pptr, int *label) {
    signed char	ch = _NXDecodeChar (coder);
    
    if (ch == NULLLABEL) {
	*pptr = NULL;
	return NO;
	};
    if (ch == NEWLABEL) {
	*label = coder->ptrCounter++;
	return YES;
	}
    else {
	IntToPtr	itp;
	
	*label = FinishDecodeInt (coder, ch);
	itp.key = *label;
	*pptr = ((IntToPtr *) NXHashGet (coder->ptrs, &itp))->value;
	return NO;
	};
    };
*/
    
//?? we have a minor leak here (8 bytes per unarchived Font or Bitmap) that could be fixed by replacing for the entry label the previous value with object, instead of allocating a new entry.  If that was done, we could check in _NXNoteShared that there was no previous value.
/*
static void _NXNoteShared (_CodingStream *coder, void *ptr, int label) {
    IntToPtr	*new = (IntToPtr *) malloc (sizeof (IntToPtr));
    
    if (! ptr) 
    	NX_RAISE (TYPEDSTREAM_INTERNAL_ERROR, 
		"_NXNoteShared: nil shared", (void *) label);
    new->key = label; new->value = ptr;
    new = NXHashInsert (coder->ptrs, new);
    if (new) free (new);
    };
*/
    
static void _NXEncodeString (_CodingStream *coder, const char *str) {
    if (_NXEncodeShared (coder, str)) _NXEncodeSharedString (coder, str);
    };

/* except in the case of nil, always returns a freshly allocated string */
static char *_NXDecodeString (_CodingStream *coder, NXZone *zone) {
  int		label;
  char	*str;
  char ch;
  switch (ch = NXGetc(coder->physical)) {
  case NULLLABEL: return NULL;
  case NEWLABEL: {
    str = _NXDecodeSharedString (coder);
    PTRS(coder->ptrs)[inc_ptrCounter(coder)] = str;
    /* we make a real copy (let's not take risks!) */
    return NXCopyStringBufferFromZone (str, zone);
  }
  default: return NXCopyStringBufferFromZone (PTRS(coder->ptrs)[Bias(FinishDecodeInt(coder, ch))], zone);
  }
}

/***********************************************************************
 *	Creation, destruction and other global operations
 **********************************************************************/

#define FIRSTSTREAMERVERSION	3
#define NXSTREAMERVERSION	4

/* The two following strings are magic: they decide of the byte oredering encoding.  It is very important that they remain small, so that only bytes are written for writting the string themselves */
#define NXSTREAMERNAME		"typedstream"
#define NXNAMESTREAMER		"streamtyped"

/* A typed stream always encodes the type of the information before its value.  Objects are specially coded because first the class is coded (along with the nbytesIndexedVars, then all its data (by sending a write: or read: message is sent to the object, and finally an end of Object token (NULLLABEL). */

NXTypedStream *NXOpenTypedStream (NXStream *physical, int mode) {
    TypedStream		*s;
    
    if (mode != NX_WRITEONLY && mode != NX_READONLY)
    	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStream: invalid mode", (void *) mode);
    if (! physical)
    	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStream: null stream", 0);
    s = (TypedStream *) malloc (sizeof (TypedStream));
    s->coder = (mode == NX_WRITEONLY) 
	? _NXOpenEncodingStream (physical) : _NXOpenDecodingStream (physical);
    s->ids = NULL;
    s->write = mode == NX_WRITEONLY;
    s->noteConditionals = NO;
    s->doStatistics = NO;
    s->fileName = NULL;
    s->classVersions = NULL;
    s->objectZone = NXDefaultMallocZone();
    if (s->write) {
	_NXEncodeChar (s->coder, NXSTREAMERVERSION);
	_NXEncodeChars (s->coder, (sameSexAsCube()) ? NXSTREAMERNAME : NXNAMESTREAMER);
	_NXEncodeInt (s->coder, NXSYSTEMVERSION);
	s->streamerVersion = NXSTREAMERVERSION;
	s->systemVersion = NXSYSTEMVERSION;
	}
    else {
    	STR	header;
	
	s->streamerVersion = _NXDecodeChar (s->coder);
	if (s->streamerVersion < FIRSTSTREAMERVERSION ||
		s->streamerVersion > NXSTREAMERVERSION) {
	    /* reading an old version or reading a non-archive file */
	    NXUngetc (s->coder->physical);
	    NXCloseTypedStream (s);
	    return NULL;
	    };
	header = _NXDecodeChars (s->coder, s->coder->scratch);
	if (strcmp (header, NXSTREAMERNAME) == 0)
	    s->coder->swap = ! sameSexAsCube();
	else if (strcmp (header, NXNAMESTREAMER) == 0)
	    s->coder->swap = sameSexAsCube();
	else {
	    /* we are not reading a typedstream file! */
	    NXCloseTypedStream (s);
	    return NULL;
	    };
	/* we are now ready for int reading */
	s->systemVersion = _NXDecodeInt (s->coder);
	s->classVersions = NXCreateHashTableFromZone (
	    NXStrStructKeyPrototype, 0, NULL, s->coder->scratch);
	free (header);
	};
    return s;
    };

extern void NXSetTypedStreamZone(NXTypedStream *stream, NXZone *zone)
{
    TypedStream	*s = (TypedStream *) stream;
    s->objectZone = zone;
}

extern NXZone *NXGetTypedStreamZone(NXTypedStream *stream)
{
    TypedStream	*s = (TypedStream *) stream;
    return s->objectZone;
}

BOOL NXEndOfTypedStream (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    checkRead (s);
    return _NXEndOfCodingStream (s->coder);
    };
    
void NXFlushTypedStream (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->coder->physical) NXFlush (s->coder->physical);
    };

void NXCloseTypedStream (NXTypedStream *stream) {
    int saveToFile = 0;
    TypedStream	*s = (TypedStream *) stream;
    if (s->write) NXFlushTypedStream (stream);
    if (s->classVersions) NXFreeHashTable (s->classVersions);
    if (s->fileName) {
	NXStream	*physical;
	physical = s->coder->physical;
	s->coder->physical = NULL;
	if (s->write) saveToFile = NXSaveToFile (physical, s->fileName);
	NXCloseMemory (physical, NX_FREEBUFFER);
	};
    _NXCloseCodingStream (s->coder);
    if (s->ids) NXFreeHashTable (s->ids);
    free (s);
    if (saveToFile) NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "file cannot be saved", NULL);
    };

/***********************************************************************
 *	Writing and reading arbitrary data: type string utilities
 **********************************************************************/

/* Writing/ reading typed values. */
/* returns a unique string */
static const char *SubTypeUntil (const char *type, char end) {
    char	*sub;
    int		level = 0;
    
    sub = buffer (strlen(type));
    while (*type) {
	switch (*type) {
	    case ']': case '}': case ')':
		if (level == 0 && *type == end) {
		    *sub = 0; 
		    return NXUniqueString (globalBuffer);
		    };
		level--;
		break;
	    case '[': case '{': case '(': level++;
	    };
	*sub ++ = *type ++;
	};
    NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, 
	"NXWriteTypes/NXReadTypes: end of type encountered prematurely", 
	(void *) (int) end);
    };

static int _SizeOfType (const char *type);

static int SizeOfSimpleType (char ch) {
    switch (ch) {
	case 'c': case 'C': return sizeof(char);
	case 's': case 'S': return sizeof(short);
	case 'i': case 'I': case 'l': case 'L': return sizeof(int);
	case 'f': return sizeof(float);
	case 'd': return sizeof (double);
	case '@': case '*': case '%': case ':': case '#': return sizeof(id);
	case '!': return sizeof (int);
	/* pointer-to-other ($), bitfield (?), selectors (: ?), shared pointers (+ ?), and regular pointers (- ?) need to be worked out! */
    	default: typeDescriptorError (ch, "unknown type descriptor");
	}
    };
    
static int SizeOfUnion (const char *type) {
    int		size = 0;
    while (*type) {
        int		s = SizeOfSimpleType (*type++);
	if (s > size) size = s;
	};
    return size;
    };

static int _SizeOfType (const char *type) {
    int		size = 0;
    char	ch;
    while (*type) switch (ch = *type++) {
	case '[': {
	    const char	*itemType;
	    int		count = 0;
	    
	    while ('0' <= *type && '9' >= *type) 
		count = count * 10 + (*type++ - '0');
	    itemType = SubTypeUntil (type, ']');
	    size += count * _SizeOfType (itemType);
	    type += strlen (itemType)+1;
	    break;
	    };
	case '{': {
	    const char	*subType = SubTypeUntil (type, '}');
	    
	    size += _SizeOfType (subType);
	    type += strlen (subType)+1;
	    break;
	    };    
	case '(': {
	    const char	*subType = SubTypeUntil (type, ')');
	    
	    size += SizeOfUnion (subType);
	    type += strlen (subType)+1;
	    break;
	    };    
    	default: size += SizeOfSimpleType (ch);
	};
    return size;
    };
    
#define next_field_addr(ARGS, TYPE)	\
	(ARGS += (sizeof(TYPE)!=1 && ((int) ARGS & 1)) ? 1 : 0,	\
	ARGS += sizeof(TYPE),					\
	(TYPE *) (ARGS - sizeof(TYPE)))

#define next_field(ARGS, TYPE)	\
	*(next_field_addr(ARGS, TYPE))
	
#define next_arg(ALIGN, ARGS, TYPE)	\
	((ALIGN) ? next_field(ARGS, TYPE) : *(va_arg(ARGS, TYPE *)))
	
/* for this one ALIGN also indicates whether with have a bunch of addresses or 1 object from which to extract addresses */
#define next_addr(ALIGN, ARGS, TYPE)	\
	((ALIGN) ? next_field_addr(ARGS, TYPE) : va_arg(ARGS, TYPE *))

/***********************************************************************
 *	Writing and reading arbitrary data: the real Write/Read of values
 **********************************************************************/

/* WriteValues returns args, properly incremented */
static va_list WriteValues (NXTypedStream *stream, const char *type, va_list args, BOOL align) {
    TypedStream	*s = (TypedStream *) stream;
    while (*type) switch (*type++) {
	case 'c': case 'C': {
	    signed char	ch = next_arg(align, args, char);
	    _NXEncodeChar (s->coder, ch);
	    break;
	    };
	case 's': case 'S': 
	    _NXEncodeShort (s->coder, next_arg(align, args, short));
	    break;
	case 'i': case 'I': case 'l': case 'L': {
	    int		x = next_arg(align, args, int);
	    _NXEncodeInt (s->coder, x);
	    break;
	    };
	case 'f': 
	    _NXEncodeFloat (s->coder, next_arg(align, args, float));
	    break;
	case 'd':
	    _NXEncodeDouble (s->coder, next_arg(align, args, double));
	    break;
	case '@':
	    InternalWriteObject (s, next_arg(align, args, id));
	    break;
	case '*': {
	    const char	*str = next_arg(align, args, char *);
	    _NXEncodeString (s->coder, str);
	    break;
	    };
	case '%': {
	    const char	*str = next_arg(align, args, char *);
	    _NXEncodeSharedString (s->coder, str);
	    break;
	    };
	case ':': {
	    SEL		sel = next_arg(align, args, SEL);
	    const char	*str = NULL;
	    if (sel) str = (const char *) sel_getName (sel);
	    _NXEncodeSharedString (s->coder, str);
	    break;
	    };
	case '#': 
	    NXWriteClass (stream, next_arg(align, args, Class));
	    break;
	case '[': {
	    const char	*itemType;
	    int		count = 0;
	    int		itemSize;
	    
	    while ('0' <= *type && '9' >= *type) 
		count = count * 10 + (*type++ - '0');
	    itemType = SubTypeUntil (type, ']');
	    while (count--) args = WriteValues (stream, itemType, args, align);
	    type += strlen (itemType)+1;
	    break;
	    };
	case '{': {
	    const char	*subType = SubTypeUntil (type, '}');
	    
	    args = WriteValues (stream, subType, args, align);
	    type += strlen (subType)+1;
	    break;
	    };    
	case '(': {
	    const char	*subType = SubTypeUntil (type, ')');
	    int		size = SizeOfUnion (subType);
	    
	    while (size--) args = WriteValues (stream, "C", args, align);
	    type += strlen (subType)+1;
	    break;
	    };    
	case '!': 
	    (void) next_arg(align, args, int);
	    break;
    	default: typeDescriptorError (*(type-1), "unknown type descriptor");
	};
    return args;
    };

/* returns args, properly incremented */
static va_list ReadValues (NXTypedStream *stream, const char *type, va_list args, BOOL align) {
    TypedStream	*s = (TypedStream *) stream;
    while (*type) switch (*type++) {
	case 'c': case 'C': {
	    signed char	*ptr = next_addr(align, args, signed char);
	    *ptr = _NXDecodeChar (s->coder);
	    break;
	    };
	case 's': case 'S': { 
	    short	*ptr = next_addr(align, args, short);
	    *ptr = _NXDecodeShort (s->coder);
	    break;
	    };
	case 'i': case 'I': case 'l': case 'L': {
	    int		*ptr = next_addr(align, args, int);
	    *ptr = _NXDecodeInt (s->coder);
	    break;
	    };
	case 'f': {
	    float	*ptr = next_addr(align, args, float);
	    *ptr = _NXDecodeFloat (s->coder);
	    break;
	    };
	case 'd': {
	    double	*ptr = next_addr(align, args, double);
	    *ptr = _NXDecodeDouble (s->coder);
	    break;
	    };
	case '@': {
	    id		*ptr = next_addr(align, args, id);
	    *ptr = InternalReadObject (s);
	    break;
	    };
	case '*': {
	    char	**ptr = next_addr(align, args, char *);
	    *ptr = _NXDecodeString (s->coder, s->objectZone);
	    break;
	    };
	case '%': {
	    char	**ptr = next_addr(align, args, char *);
	    *ptr = (char *) _NXDecodeUniqueString (s->coder);
	    break;
	    };
	case ':': {
	    SEL		*ptr = next_addr(align, args, SEL);
	    char	*selName = (char *) _NXDecodeSharedString (s->coder);
	    *ptr = _sel_registerName (selName);
	    break;
	    };
	case '#': {
	    Class	*ptr = next_addr(align, args, Class);
	    *ptr = NXReadClass (s);
	    break;
	    };
	case '[': {
	    const char	*itemType;
	    int		itemSize;
	    int		count = 0;
	    
	    while ('0' <= *type && '9' >= *type) 
		count = count * 10 + (*type++ - '0');
	    itemType = SubTypeUntil (type, ']');
	    while (count--) args = ReadValues (stream, itemType, args, align);
	    type += strlen (itemType)+1;
	    break;
	    };
	case '{': {
	    const char	*subType = SubTypeUntil (type, '}');
	    
	    args = ReadValues (stream, subType, args, align);
	    type += strlen (subType)+1;
	    break;
	    };    
	case '(': {
	    const char	*subType = SubTypeUntil (type, ')');
	    int		size = SizeOfUnion (subType);
	    
	    while (size--) args = ReadValues (stream, "C", args, align);
	    type += strlen (subType)+1;
	    break;
	    };    
	case '!': 
	    (void) next_addr(align, args, int);
	    break;
    	default: typeDescriptorError (*(type-1), "unknown type descriptor");
	};
    return args;
    };

/***********************************************************************
 *	Writing and reading arbitrary data: API functions
 **********************************************************************/

void NXWriteType (NXTypedStream *stream, const char *type, const void *data) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    _NXEncodeSharedString (s->coder, type);
    (void) WriteValues (stream, type, (void *) data, YES);
    };

void NXReadType (NXTypedStream *stream, const char *type, void *data) {
    TypedStream	*s = (TypedStream *) stream;
    const char	*readType;
    checkRead (s);
    readType = _NXDecodeSharedString (s->coder);
    checkExpected (readType, type);
    (void) ReadValues (stream, type, data, YES);
    };
    
void NXWriteTypes (NXTypedStream *stream, const char *type, ...) {
    TypedStream	*s = (TypedStream *) stream;
    va_list	args;
    
    checkWrite (s);
    va_start (args, type);
    _NXEncodeSharedString (s->coder, type);
    (void) WriteValues (stream, type, args, NO);
    va_end (args);
    };

void NXReadTypes (NXTypedStream *stream, const char *type, ...) {
    TypedStream	*s = (TypedStream *) stream;
    const char	*readType;
    va_list	args;
    checkRead (s);
    va_start (args, type);
    readType = _NXDecodeSharedString (s->coder);
    checkExpected (readType, type);
    (void) ReadValues (stream, type, args, NO);
    va_end (args);
    };
    
/***********************************************************************
 *	Conveniences for writing and reading common types of data.
 **********************************************************************/
 
void NXWriteArray (NXTypedStream *stream, const char *itemType, int count, const void *data) {
    TypedStream	*s = (TypedStream *) stream;
    char	*type = buffer (15+strlen(itemType)); /* enough room */
    
    checkWrite (s);
    sprintf (type, "[%d%s]", count, itemType);
    type = (char *) NXUniqueString (type);	/* we can now pass type around */
    _NXEncodeSharedString (s->coder, type);
    if (itemType[0] == 'c' && itemType[1] == 0) {
        _NXEncodeBytes (s->coder, (char *) data, count);
	}
    else {
	while (count--)
	    data = WriteValues (stream, itemType, (va_list) data, YES);
	};
    };

void NXReadArray (NXTypedStream *stream, const char *itemType, int count, void *data) {
    TypedStream	*s = (TypedStream *) stream;
    const char	*readType;
    char	*type = buffer (15+strlen(itemType)); /* enough room */
    
    checkRead (s);
    sprintf (type, "[%d%s]", count, itemType);
    readType = _NXDecodeSharedString (s->coder);
    checkExpected (readType, type);
    if (itemType[0] == 'c' && itemType[1] == 0) {
        _NXDecodeBytes (s->coder, (char *) data, count, FALSE);
	}
    else {
	while (count--)
	    data = ReadValues (stream, itemType, (va_list) data, YES);
	};
    };

static void InternalWriteObject (TypedStream *s, id object) {
    if ([object respondsTo:@selector(startArchiving:)])
	object = [object startArchiving: s];
    if (s->noteConditionals) {
	if (NXHashMember (s->ids, object)) return; /* already visited */
	(void) NXHashInsert (s->ids, object);
	};
    if (_NXEncodeShared (s->coder, object)) {
	int	addr;
	Class	class = object->isa;
	NXWriteClass (s, class);
	if (! s->noteConditionals && s->doStatistics)
	    addr = NXTell(s->coder->physical);
	/* a misfeature in the first version */
	if (s->streamerVersion == FIRSTSTREAMERVERSION)
	    _NXEncodeInt (s->coder, 0); 
	[object write: s];
	_NXEncodeChar (s->coder, EOOLABEL);
	if (! s->noteConditionals && s->doStatistics)
	    printf ("\tWritten %s object %s: %d bytes\n", 
		[[object class] name], [object name], 
		NXTell (s->coder->physical)-addr);
	};
    };
    
void NXWriteObject (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->streamerVersion > FIRSTSTREAMERVERSION)
	_NXEncodeSharedString (s->coder, "@");
    InternalWriteObject (s, object);
    };
    
static id InternalReadObject (TypedStream *s) {
  id object;
  char ch;
  switch (ch = NXGetc(s->coder->physical)) {
  case NULLLABEL: return NULL;
  case NEWLABEL: {
	Class	class;
	int     size;
	int label = inc_ptrCounter(s->coder);
	class = NXReadClass (s);
	if (s->streamerVersion == FIRSTSTREAMERVERSION)
	    size = _NXDecodeInt (s->coder);
	if (! class) classError ("NULL", "found null class");
	object = class_createInstanceFromZone (class, 0, s->objectZone);
	PTRS(s->coder->ptrs)[label] = object;
	[object read: s];
	[object awake];
	if ([object respondsTo:@selector(finishUnarchiving)]) {
	    id		new = [object finishUnarchiving];
	    if (new) {
	    	object = new;
		PTRS(s->coder->ptrs)[label] = object;
		};
	    };
	if (_NXDecodeChar (s->coder) != EOOLABEL) 
	    NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, 
	     "NXReadObject: inconsistency between written data and read:", 0); 
	return object;
	};
  default: return PTRS(s->coder->ptrs)[Bias(FinishDecodeInt(s->coder, ch))];
  }
}

id NXReadObject (NXTypedStream *stream) {
    const char	*readType;
    TypedStream	*s = (TypedStream *) stream;
    
    checkRead (s);
    if (s->streamerVersion > FIRSTSTREAMERVERSION) {
	readType = _NXDecodeSharedString (s->coder);
        checkExpected (readType, "@");
	};
    return InternalReadObject (s);
    };
    
//?? We hack around % classes (pose as?)
static Class RealSuperClass (Class class) {
    while ((class = class->super_class) && (class->name[0]=='%')) {};
    return class;
    };
    
static void NXWriteClass (NXTypedStream *stream, Class class) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    while (_NXEncodeShared (s->coder, class)) {
    	_NXEncodeSharedString (s->coder, class->name);
	_NXEncodeInt (s->coder, class->version);
	//?? Hack for skipping PoseAs classes (indicated by %)
	class = RealSuperClass (class);
	};
    };

static Class NXReadClass (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    char ch;
    
    checkRead (s);
    switch (ch = NXGetc(s->coder->physical)) {
    case NULLLABEL: return NULL;
    case NEWLABEL: {
      const char	*className = _NXDecodeSharedString (s->coder);
      int		version = _NXDecodeInt (s->coder);
      Class		superClass;
      Class		class = (Class) objc_getClass ((char *) className);
      _ClassVersion	*cv = (_ClassVersion *) 
	   NXZoneMalloc (s->coder->scratch, sizeof(_ClassVersion));
      /* explicit class initialization */
      (void) [(id) class self];
	cv->className = className; cv->version = version;
	(void) NXHashInsert (s->classVersions, cv);
      if (! class) classError (className, "class not loaded");
      PTRS(s->coder->ptrs)[inc_ptrCounter(s->coder)] = class;
      superClass = NXReadClass (s);
      if (superClass != RealSuperClass (class)) 
	classError (className, "wrong super class");
      return class;
    };
    default: return PTRS(s->coder->ptrs)[Bias(FinishDecodeInt(s->coder, ch))];
    }
  };
     

/***********************************************************************
 *	Writing and reading back pointers.
 **********************************************************************/

void NXWriteRootObject (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    _CodingStream	*olds;
    
    checkWrite (s);
    if (s->noteConditionals) 
	writeRefError ("NXWriteRootObject: already done");
    s->noteConditionals = YES;
    if (! s->ids) 
	s->ids = NXCreateHashTable (NXPtrPrototype, 0, NULL);
    olds = s->coder;
    s->coder = _NXOpenEncodingStream ((NXStream *) nil);
    NXWriteObject (stream, object);
    _NXCloseCodingStream (s->coder);
    s->coder = olds;
    s->noteConditionals = NO;
    NXWriteObject (stream, object);
    NXFlushTypedStream (stream);
    };

void NXWriteObjectReference (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->noteConditionals) return;
    if (! s->ids) 
	writeRefError ("NXWriteObjectReference: NXWriteRootObject has not been previously done");
    if (! object) {NXWriteObject (stream, nil); return;};
    NXWriteObject (stream, (NXHashMember (s->ids, object)) ? object : nil);
    };

/***********************************************************************
 *	Conveniences for writing and reading files and buffers.
 **********************************************************************/
 
NXTypedStream *NXOpenTypedStreamForFile (const char *fileName, int mode) {
    TypedStream	*s;
    NXStream		*physical;
    int fd;
    switch (mode) {
	case NX_WRITEONLY: 
            /* workaround NXMapFile bug */
            fd = open (fileName, O_CREAT | O_WRONLY, 0666);
            if (fd < 0) return NULL;
            close(fd);
	    physical = NXOpenMemory (NULL, 0, NX_WRITEONLY);
	    break;
	case NX_READONLY:
	    physical = NXMapFile (fileName, mode);
	    if (! physical) return NULL;
	    break;
	default: NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStreamForFile: invalid mode", (void *) mode);
	};
   s = (TypedStream *) NXOpenTypedStream (physical, mode);
   if (! s) return NULL;
   s->fileName = fileName;
   return s;
   };

char *NXWriteRootObjectToBuffer (id object, int *length) {
    char 		*buffer;
    int 		max;
    NXStream		*physical = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    NXTypedStream	*stream = NXOpenTypedStream (physical, NX_WRITEONLY);
    NXWriteRootObject (stream, object);
    NXCloseTypedStream (stream);
    NXGetMemoryBuffer(physical, &buffer, length, &max);
    NXCloseMemory(physical, NX_TRUNCATEBUFFER);
    return buffer;
    };

id NXReadObjectFromBuffer (const char *buffer, int length) {
    return NXReadObjectFromBufferWithZone(buffer, length, NXDefaultMallocZone());
    };

id NXReadObjectFromBufferWithZone (const char *buffer, int length, NXZone *zone) {
    NXStream		*physical = NXOpenMemory (buffer, length, NX_READONLY);
    NXTypedStream	*stream = NXOpenTypedStream (physical, NX_READONLY);
    id			object;
    
    NXSetTypedStreamZone(stream, zone);
    object = NXReadObject (stream);
    NXCloseTypedStream (stream);
    NXCloseMemory(physical, NX_SAVEBUFFER);
    return object;
    };

void NXFreeObjectBuffer (char *buffer, int length) {
    (void) vm_deallocate (task_self(), (vm_address_t) buffer, length);
    };


/***********************************************************************
 *	Dealing with versions
 **********************************************************************/

int NXSystemVersion (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    return s->systemVersion;
    };

int NXTypedStreamClassVersion (NXTypedStream *stream, const char *className) {
    _ClassVersion	pseudo;
    _ClassVersion	*original;
    TypedStream		*s = (TypedStream *) stream;
    checkRead (s);
    pseudo.className = className;
    original = (_ClassVersion *) NXHashGet (s->classVersions, &pseudo);
    return original->version;
    };

static short pad_to_align_global_data_to_4_byte_alignment = 0;
