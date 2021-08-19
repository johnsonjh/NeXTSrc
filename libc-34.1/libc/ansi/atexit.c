#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)exit.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>

static struct atexit_struct {
	struct atexit_struct *next;
	void (*func)(void);
} *atexit_list = NULL;

#undef atexit
int
atexit(void (*func)(void)) {
	struct atexit_struct *first;
	
	first = (struct atexit_struct *) malloc (sizeof(struct atexit_struct));
	first->next = atexit_list;
	first->func = func;
	atexit_list = first;
}

void
_atexit(void) {
	struct atexit_struct *this;
		
	for (this = atexit_list; this != NULL; this = this->next) {
		(this->func)();
	}
}
