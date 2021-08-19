/*
	perfTimer.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
	  
	DEFINED IN: The Application Kit
	HEADER FILES: perfTimer.h
  
	This file has private stuff for doing timings.
*/

#ifdef DEBUG

extern void _NXInitTimeMarker(double time);
extern void _NXSetTimeMarker(const char *str, short level);
extern void _NXClearTimeMarkers(void);
extern void _NXDumpTimeMarkers(void);
extern void _NXSetTimeMarkerWithArgs(const char *str, int arg1, int arg2, short level);
extern void _NXProfileMarker(char code, BOOL flag);

#define INITMARKTIME(time)	_NXInitTimeMarker(time)
#define MARKTIME(sym, note, level)	\
		if (sym) {_NXSetTimeMarker((note), (level));}
#define MARKTIME2(sym, note, arg1, arg2, level)	\
		if (sym) {_NXSetTimeMarkerWithArgs((note), (arg1), (arg2), (level));}
#define CLEARTIMES(sym)			\
		if (sym) {_NXClearTimeMarkers();}
#define DUMPTIMES(sym)			\
		if (sym) {_NXDumpTimeMarkers();}

#define PROFILEMARK(code, flag)		\
		_NXProfileMarker((code), (flag))

#else
#define INITMARKTIME(time)
#define MARKTIME(sym, note, level)	
#define MARKTIME2(sym, note, arg1, arg2, level)
#define CLEARTIMES(sym)	
#define DUMPTIMES(sym)
#define PROFILEMARK(code, flag)
#endif
