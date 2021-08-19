/*
	textStream.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "textprivate.h"
#import <streams/streams.h>
#import <streams/streamsimpl.h>
#import <stdlib.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryStream=0\n");
    asm(".globl .NXTextCategoryStream\n");
#endif

typedef struct {
    @defs (Text)
} TextId;

typedef struct {
    TextId             *text;
    NXTextBlock        *block;
    BOOL		invalid;
}                   textInfo;

static void resetTextStream(NXStream *s);
static int readstream_flush(register NXStream *s);
static int readstream_fill(register NXStream *s);
static void readstream_change(register NXStream *s);
static void readstream_seek(register NXStream *s, long offset);
static void readstream_close(NXStream *s);
static int defaultReadGlue(NXStream *s, void *buf, int count);
static int defaultWriteGlue(NXStream *s, const void *buf, int count);

/* INDENT OFF */
static const struct stream_functions readstream_functions = {
    defaultReadGlue,
    defaultWriteGlue,
    readstream_flush,
    readstream_fill,
    readstream_change,
    readstream_seek,
    readstream_close,
};
/* INDENT ON */

/* This glue is necessary since readstream_functions cant be initialized
   with the real NX routines because they come from a different shlib.
 */ 
static int defaultReadGlue(NXStream *s, void *buf, int count)
{
    return NXDefaultRead(s, buf, count);
}


static int defaultWriteGlue(NXStream *s, const void *buf, int count)
{
    return NXDefaultWrite(s, buf, count);
}

@implementation Text(Stream)

- invalidateStream
{
    textInfo *info = (textInfo *) textStream->info;
    info->invalid = YES;
    return self;
}

- validateStream
{
    textInfo *info = (textInfo *) textStream->info;
    if (info->invalid)
	resetTextStream(textStream);
    return self;
}

@end


/*
 *	readstream_flush:  flush a stream buffer.
 */

static int readstream_flush(register NXStream *s)
{
    return 0;
}

static int readstream_fill(register NXStream *s)
{
    textInfo           *info = (textInfo *) s->info;
    NXTextBlock        *next;
    BOOL		lastBlock;

    if (info->invalid) {
	int pos = NXTell(s);
	resetTextStream(s);
	NXSeek(s, pos, NX_FROMSTART);
    }
    next = info->block->next;
    if (!next)
	return 0;
    s->buf_base = next->text;
    s->buf_ptr = s->buf_base;
    lastBlock = (next == info->text->lastTextBlock);
    s->buf_size = next->chars - (lastBlock ? 1:0);
    s->buf_left = s->buf_size;
    info->block = next;
    return (s->buf_size);
}

static void readstream_change(register NXStream *s)
{
}

static void readstream_seek(register NXStream *s, long offset)
{
    textInfo           *info = (textInfo *) s->info;
    int                 delta;
    NXTextBlock        *block;

    if (info->invalid || !offset)
	resetTextStream(s);
    if (offset >= s->offset && offset < (s->offset + s->buf_size)) {
	delta = offset - s->offset;
    } else {
	block = info->block;
	if (offset < s->offset) {
	    do {
		block = block->prior;
		s->offset -= block->chars;
	    } while (offset < s->offset);
	} else {
	    while (offset >= (s->offset + block->chars)) {
		s->offset += block->chars;
		block = block->next;
	    }
	}
	info->block = block;
	s->buf_base = block->text;
	if (block->next)
	    s->buf_size = block->chars;
	else
	    s->buf_size = block->chars - 1;
	delta = offset - s->offset;
    }
    s->buf_ptr = s->buf_base + delta;
    s->buf_left = s->buf_size - delta;
}

static void readstream_close(NXStream *s)
{
}

extern NXStream    *_NXOpenTextStream(id self)
{
    NXStream           *s;
    TextId             *text = (TextId *) self;
    textInfo           *info;
    NXZone *zone = [self zone];

    s = (NXStream *)NXStreamCreateFromZone(NX_READONLY, 0, zone);
    s->functions = &readstream_functions;
    info = (textInfo *) NXZoneMalloc(zone, sizeof(textInfo));
    s->info = (char *)info;
    info->text = text;
    resetTextStream(s);
    return (s);
}


static void resetTextStream(NXStream *s)
{
    textInfo           *info = (textInfo *) s->info;
    NXTextBlock        *block = info->text->firstTextBlock;

    s->buf_base = block->text;
    if (block->next)
	s->buf_size = block->chars;
    else
	s->buf_size = block->chars - 1;
    s->buf_left = s->buf_size;
    s->buf_ptr = s->buf_base;
    s->eof = info->text->textLength - 1;
    s->offset = 0;
    s->flags &= ~NX_EOS;
    if (s->eof == s->offset)
	s->flags |= NX_EOS;
    info->invalid = NO;
    info->block = block;
}

/***

94
--
 9/25/90 gcockrft	Removed _NXGetPrevious(NXStream *s)
 
 **/

