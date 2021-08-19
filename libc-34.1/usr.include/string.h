/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _STRING_H
#define _STRING_H

#ifdef __STRICT_BSD__
#include <strings.h>
extern char *strcpyn();
extern char *strcatn();
extern int strcmpn();
extern char *strchr();
extern char *strrchr();
extern char *strpbrk();
extern int strspn();
extern int strcspn();
extern char *strtok();
#else /* __STRICT_BSD__ */

#include <stddef.h>

extern void *memcpy(void *s1, const void *s2, size_t n);
extern void *memmove(void *s1, const void *s2, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern size_t strcoll(char *to, size_t maxsize, const char *from);
extern void *memchr(const void *s, int c, size_t n);
extern char *strstr(const char *s1, const char *s2);
extern void *memset(void *s, int c, size_t n);
extern char *strerror(int errnum);
extern char *strcpy(char *s1, const char *s2);
extern char *strcat(char *s1, const char *s2);
extern int strcmp(const char *s1, const char *s2);
extern int strcasecmp(const char *s1, const char *s2);
extern char *strchr(const char *s, int c);
extern char *strpbrk(const char *s1, const char *s2);
extern char *strrchr(const char *s, int c);
extern char *strtok(char *s1, const char *s2);
extern char *strncpy(char *s1, const char *s2, size_t n);
extern char *strncat(char *s1, const char *s2, size_t n);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern int strncasecmp(const char *s1, const char *s2, size_t n);
extern size_t strcspn(const char *s1, const char *s2);
extern size_t strspn(const char *s1, const char *s2);
extern size_t strlen(const char *s);

#ifndef __STRICT_ANSI__
/* added for BSD compatibility */
#undef index
extern char *index(const char *s, int c);
#define index(s,c) strchr(s,c)

#undef rindex
extern char *rindex(const char *s, int c);
#define rindex(s,c) strrchr(s,c)

#define bcopy(from,to,len) ((void)memmove(to,from,len))
#define bcmp(s1,s2,len) memcmp(s1,s2,len)
#define bzero(b,len) memset(b,0,len)

/* obsolete trash, for BSD compatibility */
extern char *strcpyn();
extern char *strcatn();
extern int strcmpn();
#endif /* __STRICT_ANSI__ */
#endif /* __STRICT_BSD__ */

#endif /* _STRING_H */
