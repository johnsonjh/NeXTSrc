/* user defined */
extern char *progname;
extern void cleanup(int sig);

/* defined in errors.c */
extern long errors;	/* number of detected calls to error() */
extern void error(const char *format, ...);
extern void perror_error(const char *format, ...);
extern void fatal(const char *format, ...);
extern void perror_fatal(const char *format, ...);
extern void mach_fatal(kern_return_t r, char *format, ...);
