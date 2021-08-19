#import <objc/hashtable.h>
#import <libc.h>
#import <appkit/Font.h>
#import "TXText.h"
#import "confirm.h"
#import "super.h"

typedef struct {
	id	font;
	float	in1;
	float	in2;
	NXTextStyle	*style;
} HashEntry;

typedef struct {
	@defs (Font)
} _IDFont ;

#define STYLE(f) (((_IDFont *)(f))->style)
#define STYLE_CMP(font1, style1, font2, style2) \

static NXTextStyle *dfltParaStyle = (NXTextStyle *)0;
static NXRunArray *NroffRuns = NULL;
static int sizeofOneRun = sizeof(NXRun);
static float in1=0., in2=0.;
static NXHashTable *styleTable = NULL;

extern id textFont[];	 /* in NroffText.m */

static unsigned styleHash(const void *info, const void *data)
{
	HashEntry *d = (HashEntry *)data;
	return NXPtrHash(info, (void *)((long)d->font + 
					(long)d->in1 + (long)d->in2));
}

static int styleIsEqual(const void *info, const void *data1, const void *data2)
{
	HashEntry *d1 = (HashEntry *)data1;
	HashEntry *d2 = (HashEntry *)data2;

	return(d1->font == d2->font
		&& STYLE(d1->font) == STYLE(d2->font)
		&& d1->in1 == d2->in1
		&& d1->in2 == d2->in2);
}

static void styleFree(const void *info, void *data)
{
	HashEntry *d = (HashEntry *)data;
	if (!d) return;
	if (d->style) free(d->style);
	free(d);
}

static NXHashTablePrototype styleTableProto = {styleHash, styleIsEqual, styleFree, 0};

static NXTextStyle *makeStyle(NXTextStyle *prevStyle, id fontId, float i1, float i2)
{
	NXTextStyle	*currStyle;

	if (!prevStyle) return dfltParaStyle;
	currStyle = (NXTextStyle *)malloc(sizeof(NXTextStyle));
	if (!currStyle) return dfltParaStyle;
	*currStyle = *(NXTextStyle *)prevStyle;

	currStyle->indent1st = i1;
	currStyle->indent2nd = i2;
	return(currStyle);
}

static growRunbyOne()
{	
	if (!NroffRuns) {
		NroffRuns = (NXRunArray *)NXChunkMalloc(0, sizeofOneRun);
		NroffRuns->chunk.used = sizeofOneRun;

	}
	else {
		if ((NroffRuns->chunk.used + sizeofOneRun) >= 
		    NroffRuns->chunk.allocated)
		    NroffRuns = (NXRunArray *)NXChunkRealloc(
			&(NroffRuns->chunk));
		NroffRuns->chunk.used += sizeofOneRun;
	}
}

NXTextStyle *TXsetParaStyle (NXTextStyle *prevStyle, id font, float in1, float in2)
{
	HashEntry	search;
	HashEntry	*entry;
	
	if (!styleTable) {
		styleTable = NXCreateHashTable(styleTableProto, 0, NULL);
	}
	
	search.font = font;
	search.in1 = in1;
	search.in2 = in2;
	entry = (HashEntry *)NXHashGet(styleTable, (void *)&search);
	if (!entry) {
		entry = (HashEntry *)malloc(sizeof(HashEntry));
		*entry = search;
		entry->style = makeStyle(prevStyle, font, in1, in2);
		(void)NXHashInsert(styleTable, (void *)entry);
	}
	return entry->style;
}

void TXsetDfltParaStyle(NXTextStyle *newStyle)
{
	dfltParaStyle = newStyle;
}

void TXfreeRuns()
{
	if (NroffRuns) {
		NroffRuns->chunk.used = 0;
		NroffRuns->runs[0].chars = 0;
	}
}

void TXaddRun(id font, int chars)
{	
	int r = (NroffRuns ? NroffRuns->chunk.used/sizeof(NXRun) : 0);
	NXTextStyle *newStyle = TXsetParaStyle(dfltParaStyle, font, in1, in2);
	NXRun *runP;
	
	if (chars <= 0) return;
	
	/* if it's really the same style as before then extend
	 * the previous run by chars.
	 */
	if (r) {
		runP = &(NroffRuns->runs[r-1]);
		if (newStyle == runP->paraStyle) {
			runP->chars += chars;
			return;
		}
	}
	/* otherwise we need to add a new run */
	growRunbyOne();
	runP = &(NroffRuns->runs[r]);
	bzero(runP, sizeof(NXRun));
	runP->chars = chars;
	runP->font = font;
	runP->paraStyle = newStyle;
	runP->superscript = runP->subscript = 0;
	if (STYLE(font) == NX_SUPERSCRIPT)
		runP->superscript = (int)([font pointSize]/2.5);
	else
	if (STYLE(font) == NX_SUBSCRIPT)
		runP->subscript = (int)([font pointSize]/2.5);
}

void TXsecondIndent(float n)
{
	in2 = n;
}

void TXfirstIndent(float n)
{
	in1 = n;
}

float TXstrWidth(char *s, int f)
{
	return [textFont[f] getWidthOf:s];
}

id TXtextFont(int type)
{
	return textFont[type];
}

NXRunArray *TXnroffRuns()
{
	return NroffRuns;
}

int TXsetTextFont(int type, id font)
{
	int r = (NroffRuns ? NroffRuns->chunk.used/sizeof(NXRun) : 0);
	id oldFont = textFont[type];
	int changed = 0, i;

	for(i=0; i < r; i++) {
		if (NroffRuns->runs[i].font == oldFont) {
			NroffRuns->runs[i].font = font;
			changed++;
		}
	}
	
	textFont[type] = font;
	return changed;
}

