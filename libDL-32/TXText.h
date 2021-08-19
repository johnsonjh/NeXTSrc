#import <objc/objc.h>
#import <appkit/Text.h>

#define fBoldItalic 0
#define fPlain	1
#define fBold	2
#define fEntry  fBold
#define fItalic	3
#define fSmall	4
#define fSuper	5
#define fSub	6
#define fSymbol	7
#define fPrev	8
#define fLexi	11
#define fSanSerif 12
#define fNUM	16

extern NXTextStyle *TXsetParaStyle (NXTextStyle *prevStyle, id font, float in1, float in2);
extern void TXsetDfltParaStyle(NXTextStyle *newStyle);
extern void TXfreeRuns(void);
extern void TXaddRun(id font, int chars);
extern void TXsecondIndent(float n);
extern void TXfirstIndent(float n);
extern float TXstrWidth(char *s, int f);
extern id TXtextFont(int type);
extern NXRunArray *TXnroffRuns(void);
extern int TXsetTextFont(int type, id font);

