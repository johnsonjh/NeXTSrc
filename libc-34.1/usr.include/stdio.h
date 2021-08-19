/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)stdio.h	5.3 (Berkeley) 3/15/86
 */

#ifndef _STDIO_H
#define _STDIO_H

#ifdef __STRICT_BSD__
#ifndef NULL
#define	NULL	0
#endif 
#else
#include <stddef.h> /* get NULL, errno */
#include <stdarg.h> /* get va_list */
#endif

#define	BUFSIZ	1024
extern	struct	_iobuf {
	int	_cnt;
	char	*_ptr;		/* should be unsigned char */
	char	*_base;		/* ditto */
	int	_bufsiz;
	short	_flag;
#ifdef	__NeXT__
	unsigned char	_file;	/* should be short */
#else
	char	_file;		/* should be short */
#endif	__NeXT__
	char	_smallbuf;	/* character for unbuf file */
} _iob[];

#define	_IOREAD	01
#define	_IOWRT	02
#define	_IONBF	04
#define	_IOMYBUF	010
#define	_IOEOF	020
#define	_IOERR	040
#define	_IOSTRG	0100
#define	_IOLBF	0200
#define	_IORW	0400
#define	FILE	struct _iobuf
#define	EOF	(-1)

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])

#ifdef __STRICT_BSD__
extern char *bsd_sprintf();
extern char *bsd_vsprintf();
#define sprintf bsd_sprintf
#define vsprintf bsd_vsprintf
extern FILE *fopen();
extern FILE *freopen();
extern long int ftell();
extern char *fgets();
extern char *gets();
#else
typedef long fpos_t;

#define _IOFBF	00		/* any value not equal to LBF or NBF */
#define L_tmpnam 14		/* large enough to hold tmpnam result */
#define FOPEN_MAX 256		/* min files guaranteed open simultaneously */
#define FILENAME_MAX 1024	/* max len string that can be opened as file */
#define SEEK_SET 0		/* arguments to fseek function */
#define SEEK_CUR 1
#define SEEK_END 2
#define TMP_MAX 25		/* min unique file names from tmpnam */

extern int remove(const char *filename);
extern int rename(const char *old, const char *new);
extern FILE *tmpfile(void);
extern char *tmpnam(char *s);
extern int fclose(FILE *stream);
extern int fflush(FILE *stream);
extern void setbuf(FILE *stream, char *buf);
extern int setvbuf(FILE *stream, char *buf, int mode, size_t size);
extern int fprintf(FILE *stream, const char *format, ...);
extern int fscanf(FILE *stream, const char *format, ...);
extern int printf(const char *format, ...);
extern int scanf(const char *format, ...);
extern int sprintf(char *s, const char *format, ...);
extern int sscanf(const char *s, const char *format, ...);
extern int vfprintf(FILE *stream, const char *format, va_list arg);
extern int vprintf(const char *format, va_list arg);
extern int vsprintf(char *s, const char *format, va_list arg);
extern int fgetc(FILE *stream);
extern int fputc(int c, FILE *stream);
extern int fputs(const char *s, FILE *stream);
extern int getc(FILE *stream);
extern int _flsbuf();
extern int _filbuf();
extern int getchar(void);
extern int putc(int c, FILE *stream);
extern int putchar(int c);
extern int puts(const char *s);
extern int ungetc(int c, FILE *stream);
extern size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern int fgetpos(FILE *stream, fpos_t *pos);
extern int fseek(FILE *stream, long int offset, int whence);
extern int fsetpos(FILE *stream, const fpos_t *pos);
extern void rewind(FILE *stream);
extern void clearerr(FILE *stream);
extern int feof(FILE *stream);
extern int ferror(FILE *stream);
extern void perror(const char *s);
extern FILE *fopen(const char *filename, const char *mode);
extern FILE *freopen(const char *filename, const char *mode, FILE *stream);
extern long int ftell(FILE *stream);
extern char *fgets(char *s, int n, FILE *stream);
extern char *gets(char *s);
#endif /* __STRICT_BSD */

#ifndef lint
#define	getc(p)		(--(p)->_cnt>=0? (int)(*(unsigned char *)(p)->_ptr++):_filbuf(p))
#endif /* not lint */
#define	getchar()	getc(stdin)
#ifndef lint
#define putc(x, p)	(--(p)->_cnt >= 0 ?\
	(int)(*(unsigned char *)(p)->_ptr++ = (x)) :\
	(((p)->_flag & _IOLBF) && -(p)->_cnt < (p)->_bufsiz ?\
		((*(p)->_ptr = (x)) != '\n' ?\
			(int)(*(unsigned char *)(p)->_ptr++) :\
			_flsbuf(*(unsigned char *)(p)->_ptr, p)) :\
		_flsbuf((unsigned char)(x), p)))
#endif /* not lint */
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	clearerr(p)	((p)->_flag &= ~(_IOERR|_IOEOF))

#ifndef __STRICT_ANSI__
/* BSD compatibility */
extern int fileno(FILE *stream);
#define	fileno(p)	((p)->_file)

#ifdef __STRICT_BSD__
FILE	*fdopen();
FILE	*popen();
extern int pclose();
#else
FILE	*fdopen(int filedes, const char *mode);
FILE	*popen(const char *command, const char *mode);
extern int pclose(FILE *stream);
#endif /* __STRICT_BSD__ */
#endif /* __STRICT_ANSI__ */
#endif /* _STDIO_H */
