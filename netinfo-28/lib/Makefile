SRCS= system.c socket_lock.c
HDRS= clib.h mm.h system.h socket_lock.h

-include Makefile.optimize

OBJS= $(SRCS:.c=.o)
CFLAGS= $(OPTIMIZE) -I../lib

LIBNI= libni.a

all: $(LIBNI)

$(LIBNI): $(OBJS)
	ar cr $@ $(OBJS)
	ranlib $@

clean:
	-rm -f *.o *.out *~ $(LIBNI)

install: $(LIBNI)
	@echo -n ""

installsrc: $(SRCS) $(HDRS) Makefile Makefile.optimize
	cp $? $(SRCROOT)




	
