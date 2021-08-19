extern int confirmToStderr;

extern int vConfirm(char *okBtn, char *fmt, va_list ap);
extern int confirm(char *okBtn, char *fmt, ...);
extern void vError(char *fmt, va_list ap);
extern void error(char *fmt, ...);
extern void sysError(char *str);
