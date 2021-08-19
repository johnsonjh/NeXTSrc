
#define shutdown __shutdown__

#include <libc.h>

#undef roundup
#undef shutdown

extern int getppid(void);
extern int pmap_unset(unsigned, unsigned);
extern bool_t xdr_free(xdrproc_t, void *);
extern enum clnt_stat _many_cast_args(unsigned,
				      struct in_addr *, u_long, u_long, u_long,
				      bool_t (*)(), void *, unsigned, 
				      bool_t (*)(), 
				      void *,
				      bool_t (*)(), int);

extern const char *mach_errormsg(kern_return_t);

