#include <libc.h>
enum clnt_stat many_cast_args(unsigned,
			      struct in_addr *, u_long, u_long, u_long,
			      bool_t (*)(), void *, unsigned, bool_t (*)(), 
			      void *,
			      bool_t (*)(), int);
int pmap_unset(unsigned, unsigned);
bool_t xdr_free(xdrproc_t, void *);
