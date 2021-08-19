/*
	nibprivate.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Jean-Marie Hullot
  
	DEFINED IN: The Application Kit
*/

#import <objc/objc.h>

#define HASHSIZE	31
#define HASHFUN(s)	_strhash(s)

extern unsigned 	_strhash();

typedef struct _NameRecord {
    char        *name;
    id          object;
    id		owner;
}               NameRecord, *NamePtr;

#define NAMESIZE	(sizeof(NameRecord))
#define NAMEDESCR	"{*@@}"

typedef struct _ConnectRecord {
    int		type;
    char        *label;
    id          source;
    id		dest;
    id		extension;
}               ConnectRecord, *ConnectPtr;

#define CONNECTSIZE	(sizeof(ConnectRecord))
#define CONNECTDESCR	"{i*@@@}"

#define CONTROL		0
#define OUTLET		1

/* Archiving */

#define MAINDATAENTRY	1
#define	MAINIMAGEENTRY	2
#define	MAINSOUNDENTRY	3
#define MAINCLASSENTRY	4
#define MAINPROJECTENTRY	5
#define DEVONLYENTRY	6

