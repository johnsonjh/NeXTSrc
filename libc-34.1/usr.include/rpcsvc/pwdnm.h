/*	@(#)pwdnm.h	1.2 88/05/08 4.0NFSSRC SMI	*/ /* c2 secure */

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.2
 */

struct pwdnm {
	char *name;
	char *password;
};
typedef struct pwdnm pwdnm;


#define PWDAUTH_PROG 100036
#define PWDAUTH_VERS 1
#define PWDAUTHSRV 1
#define GRPAUTHSRV 2

bool_t xdr_pwdnm();
