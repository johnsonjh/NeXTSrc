/*
	typedstreamprivate.h
	Copyright 1989 NeXT, Inc.
	Responsibility: Bertrand Serlet
*/

/*
 *	This module provides the type definitions necessary for typedstream.h
 *	No client except typedstream should directly use this module.
 *
 */
 

#import <streams/streams.h>
#import "objc.h"
#import "hashtable.h"


/*************************************************************************
 *	Low-Level: encoding tightly information, 
 *	and sharing pointers and strings
 **************************************************************************/


typedef struct {
    NXStream	*physical;	/* the underlying stream */
    BOOL	swap;		/* should be swap bytes on reading */
    BOOL	write;		/* are we writing */
    NXHashTable	*strings;	/* maps strings to labels (vice versa for reading) */
    int		stringCounter;	/* next string label */
    int		stringCounterMax;
    NXHashTable	*ptrs;		/* maps ptrs to labels (vice versa for reading) */
    int		ptrCounter;	/* next ptr label */
    int		ptrCounterMax;
    NXZone	*scratch;
    } _CodingStream;

/* Creation, destruction */
extern _CodingStream *_NXOpenEncodingStream (NXStream *physical);
	/* creates an encoding stream, given physical stream */

extern _CodingStream *_NXOpenDecodingStream (NXStream *physical);
	/* creates an decoding stream, given physical stream */

extern BOOL _NXEndOfCodingStream (_CodingStream *coder);
	/* TRUE iff end of stream */

extern void _NXCloseCodingStream (_CodingStream *coder);

/* Encoding/Decoding of usual quantities */

extern void _NXEncodeBytes (_CodingStream *coder, const char *buf, int count);
extern void _NXDecodeBytes (_CodingStream *coder, char *buf, int count, BOOL swap);

extern void _NXEncodeChar (_CodingStream *coder, signed char c);
extern signed char _NXDecodeChar (_CodingStream *coder);

extern void _NXEncode (_CodingStream *coder, short x);
extern short _NXDecodeShort (_CodingStream *coder);

extern void _NXEncodeInt (_CodingStream *coder, int x);
extern int _NXDecodeInt (_CodingStream *coder);

extern void _NXEncodeFloat (_CodingStream *coder, float x);
extern float _NXDecodeFloat (_CodingStream *coder);

extern void _NXEncodeDouble (_CodingStream *coder, double x);
extern double _NXDecodeDouble (_CodingStream *coder);

/* low-level string coding; should never be called directly */
extern void _NXEncodeChars (_CodingStream *coder, const char *str);
extern char *_NXDecodeChars (_CodingStream *coder, NXZone *zone);

/* Encoding/Decoding of shared quantities.  Forces sharing of strings: identical but non shared strings at encoding will end up shared at decoding */
extern void _NXEncodeSharedString (_CodingStream *coder, const char *str);
extern char *_NXDecodeSharedString (_CodingStream *coder);
extern const char *_NXDecodeUniqueString (_CodingStream *coder);
	/* always returns a "unique" string, that should never be freed */
	
extern void _NXEncodeString (_CodingStream *coder, const char *str);
extern char *_NXDecodeString (_CodingStream *coder, NXZone *zone);
	/* always returns a new string */
	
extern BOOL _NXEncodeShared (_CodingStream *coder, const void *ptr);
	/* if ptr was previously encoded, encodes its previous label and returns FALSE
	if this is a new ptr, encodes that it's a new one, increases the label counter and returns TRUE ; It is assumed that the information relative to ptr is then encoded */
extern BOOL _NXDecodeShared (_CodingStream *coder, void **pptr, int *label);
	/* if we are reading a previously read quantity or nil, its previous ptr is set, and FALSE is returned
	if we are reading a new marker, sets the appropriate label and returns TRUE.  The data should then be read and _NXNoteShared should be called */
extern void _NXNoteShared (_CodingStream *coder, void *ptr, int label);

/*************************************************************************
 *	Encoding typed information, and dealing with ids
 **************************************************************************/

typedef struct {
    const char		*className;
    int			version;
    } _ClassVersion;

typedef struct {
    _CodingStream	*coder; /* the underlying stream */
    NXHashTable		*ids;	/* Set of all visited IDs */
    BOOL		write;	/* writing vs reading */
    BOOL		noteConditionals; /* when TRUE, it's a no-write pass */
    BOOL		doStatistics ; /* prints statistics */
    const char		*fileName; /* unless nil, file to be written when stream is closed */
    signed char		streamerVersion; /* changes in the meta-schema */
    int			systemVersion;	/* typically, appkit version */
    NXHashTable		*classVersions;	/* for read: [className -> version] */
    int			classVersionsCounter;
    NXZone		*objectZone;
    } TypedStream;

extern void InternalWriteObject (TypedStream *s, id object);
	/* writes an object without header */
extern id InternalReadObject (TypedStream *s);
	/* reads an object without header */

extern void NXWriteClass (NXTypedStream *stream, Class class);
	/* Equivalent to NXWriteTypes (stream, "#", &class) */
extern Class NXReadClass (NXTypedStream *stream);
	

