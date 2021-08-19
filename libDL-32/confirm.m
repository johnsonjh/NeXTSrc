#import <stdio.h>
#import <string.h>
#import "confirm.h"
#import <appkit/Application.h>
#import <appkit/Panel.h>

int             confirmToStderr = 0;

int
vConfirm(char *okBtn, char *fmt, va_list ap)
{
	char            str[1024];

	vsprintf(str, fmt, ap);
	if (!okBtn) okBtn = "OK";
#ifdef VERS92
	return NXAlert(str, okBtn, "Cancel", (char *)0);
#else
	/* these actually take printf msgs themselves */
	return NXRunAlertPanel(NULL, str, okBtn, "Cancel", (char *)0);
#endif
}

int
confirm(char *okBtn, char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vConfirm(okBtn, fmt, ap);
	va_end(ap);
	return ret;
}

void
vError(char *fmt, va_list ap)
{
	char            str[1024];

	vsprintf(str, fmt, ap);
#ifdef VERS92
	NXAlert(str, "OK", (char *)0, (char *)0);
#else
	/* these actually take printf msgs themselves */
	NXRunAlertPanel(NULL, str, "OK", (char *)0, (char *)0);
#endif
}

void
error(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	if (confirmToStderr) {
		fprintf(stderr, "%s:", [NXApp appName]);
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	}
	else {
		vError(fmt, ap);
	}
	va_end(ap);
}

void
sysError(char *str)
{
	if (confirmToStderr) {
		perror(str);
	}
	error("%s: %s", str, strerror(errno));
}
