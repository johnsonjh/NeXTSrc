/*
 * Useful macros and other stuff for generic lookups
 * Copyright (C) 1989 by NeXT, Inc.
 */

#import <netinfo/lookup_types.h>

extern port_t _lu_port;
extern unit *_lookup_buf;
extern int _lu_running(void);


typedef enum lookup_state {
	LOOKUP_CACHE,
	LOOKUP_FILE,
} lookup_state;

#define SETSTATE(_lu_set, _old_set, state, stayopen) \
{ \
	if (_lu_running()) { \
		_lu_set(stayopen); \
		*state = LOOKUP_CACHE; \
	} else { \
		_old_set(stayopen); \
		*state = LOOKUP_FILE; \
	} \
} 

#define UNSETSTATE(_lu_unset, _old_unset, state) \
{ \
	if (_lu_running()) { \
		_lu_unset(); \
	} else { \
		_old_unset(); \
	} \
	*state = LOOKUP_CACHE; \
}

#define GETENT(_lu_get, _old_get, state, res_type) \
{ \
	res_type *res; \
\
	if (_lu_running()) { \
		if (*state == LOOKUP_CACHE) { \
			res = _lu_get(); \
		} else { \
			res = _old_get(); \
		} \
	} else { \
		res = _old_get(); \
	} \
	return (res); \
}

#define LOOKUP1(_lu_lookup, _old_lookup, arg, res_type) \
{ \
	res_type *res; \
 \
	if (_lu_running()) { \
		res = _lu_lookup(arg); \
	} else { \
		res = _old_lookup(arg); \
	} \
	return (res); \
}

#define LOOKUP2(_lu_lookup, _old_lookup, arg1, arg2, res_type) \
{ \
	res_type *res; \
 \
	if (_lu_running()) { \
		res = _lu_lookup(arg1, arg2); \
	} else { \
		res = _old_lookup(arg1, arg2); \
	} \
	return (res); \
}


extern void bzero(void *, unsigned);
extern void bcopy(void *, void *, unsigned);
extern void xdr_free(int (*xdr_func)(), void *);
#ifndef htonl
extern unsigned long htonl(unsigned long);
#endif

