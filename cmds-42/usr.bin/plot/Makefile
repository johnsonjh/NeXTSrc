#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= tek t4013 t300 t300s t450 aedplot bgplot crtplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s -lm

HFILES=
YFILES=
CFILES= atoplot.c chrtab.c crtdriver.c crtplot.c debug.c driver.c plottoa.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	for i in ${PRODUCTS}; do \
		(install ${IFLAGS} $${i}.NEW $(DSTROOT)$(BINDIR)/$$i); done
	install -c -xs -m 555 plot.sh $(DSTROOT)$(BINDIR)/plot

#
# Building tek
#
TEK_OBJS= driver.o

tek.NEW:	${TEK_OBJS} tek_vers.o
	$(CC) $(CFLAGS) -o $@ ${TEK_OBJS} tek_vers.o ${LIBS} -l4014

tek_vers.c: ${DOVERS} ${TEK_OBJS}
	@rm -f $@
	vers_string -c tek > $@

#
# Building t4013
#
T4013_OBJS= driver.o

t4013.NEW:	${T4013_OBJS} t4013_vers.o
	$(CC) $(CFLAGS) -o $@ ${T4013_OBJS} t4013_vers.o ${LIBS} -l4013

t4013_vers.c: ${DOVERS} ${T4013_OBJS}
	@rm -f $@
	vers_string -c t4013 > $@

#
# Building t300
#
T300_OBJS= driver.o

t300.NEW:	${T300_OBJS} t300_vers.o
	$(CC) $(CFLAGS) -o $@ ${T300_OBJS} t300_vers.o ${LIBS} -l300

t300_vers.c: ${DOVERS} ${T300_OBJS}
	@rm -f $@
	vers_string -c t300 > $@

#
# Building t300s
#
T300S_OBJS= driver.o

t300s.NEW:	${T300S_OBJS} t300s_vers.o
	$(CC) $(CFLAGS) -o $@ ${T300S_OBJS} t300s_vers.o ${LIBS} -l300s

t300s_vers.c: ${DOVERS} ${T300S_OBJS}
	@rm -f $@
	vers_string -c t300s > $@

#
# Building t450
#
T450_OBJS= driver.o

t450.NEW:	${T450_OBJS} t450_vers.o
	$(CC) $(CFLAGS) -o $@ ${T450_OBJS} t450_vers.o ${LIBS} -l450

t450_vers.c: ${DOVERS} ${T450_OBJS}
	@rm -f $@
	vers_string -c t450 > $@

#
# Building aedplot
#
AEDPLOT_OBJS= driver.o

aedplot.NEW:	${AEDPLOT_OBJS} aedplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${AEDPLOT_OBJS} aedplot_vers.o ${LIBS} -lplotaed

aedplot_vers.c: ${DOVERS} ${AEDPLOT_OBJS}
	@rm -f $@
	vers_string -c aedplot > $@

#
# Building bgplot
#
BGPLOT_OBJS= driver.o

bgplot.NEW:	${BGPLOT_OBJS} bgplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${BGPLOT_OBJS} bgplot_vers.o ${LIBS} -lplotbg

bgplot_vers.c: ${DOVERS} ${BGPLOT_OBJS}
	@rm -f $@
	vers_string -c bgplot > $@

#
# Building crtplot
#
CRTPLOT_OBJS= crtplot.o crtdriver.o

crtplot.NEW:	${CRTPLOT_OBJS} crtplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${CRTPLOT_OBJS} crtplot_vers.o ${LIBS}\
		-ltermcap -lcurses

crtplot_vers.c: ${DOVERS} ${CRTPLOT_OBJS}
	@rm -f $@
	vers_string -c crtplot > $@

#
# Building dumbplot
#
DUMBPLOT_OBJS= driver.o

dumbplot.NEW:	${DUMBPLOT_OBJS} dumbplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${DUMBPLOT_OBJS} dumbplot_vers.o ${LIBS} \
		-lplotdumb -ltermcap

dumbplot_vers.c: ${DOVERS} ${DUMBPLOT_OBJS}
	@rm -f $@
	vers_string -c dumbplot > $@

#
# Building gigiplot
#
GIGIPLOT_OBJS= driver.o

gigiplot.NEW:	${GIGIPLOT_OBJS} gigiplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${GIGIPLOT_OBJS} gigiplot_vers.o ${LIBS} \
		-lplotgigi

gigiplot_vers.c: ${DOVERS} ${GIGIPLOT_OBJS}
	@rm -f $@
	vers_string -c gigiplot > $@

#
# Building hpplot
#
HPPLOT_OBJS= driver.o

hpplot.NEW:	${HPPLOT_OBJS} hpplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${HPPLOT_OBJS} hpplot_vers.o ${LIBS} -lplot2648

hpplot_vers.c: ${DOVERS} ${HPPLOT_OBJS}
	@rm -f $@
	vers_string -c hpplot > $@

#
# Building hp7221plot
#
HP7221PLOT_OBJS= driver.o

hp7221plot.NEW:	${HP7221PLOT_OBJS} hp7221plot_vers.o
	$(CC) $(CFLAGS) -o $@ ${HP7221PLOT_OBJS} hp7221plot_vers.o ${LIBS} \
		-lplot7221

hp7221plot_vers.c: ${DOVERS} ${HP7221PLOT_OBJS}
	@rm -f $@
	vers_string -c hp7221plot > $@

#
# Building implot
#
IMPLOT_OBJS= driver.o

implot.NEW:	${IMPLOT_OBJS} implot_vers.o
	$(CC) $(CFLAGS) -o $@ ${IMPLOT_OBJS} implot_vers.o ${LIBS} -lplotimagen

implot_vers.c: ${DOVERS} ${IMPLOT_OBJS}
	@rm -f $@
	vers_string -c implot > $@

#
# Building atoplot
#
ATOPLOT_OBJS= atoplot.o

atoplot.NEW:	${ATOPLOT_OBJS} atoplot_vers.o
	$(CC) $(CFLAGS) -o $@ ${ATOPLOT_OBJS} atoplot_vers.o ${LIBS} -lplot

atoplot_vers.c: ${DOVERS} ${ATOPLOT_OBJS}
	@rm -f $@
	vers_string -c atoplot > $@

#
# Building plottoa
#
PLOTTOA_OBJS= plottoa.o

plottoa.NEW:	${PLOTTOA_OBJS} plottoa_vers.o
	$(CC) $(CFLAGS) -o $@ ${PLOTTOA_OBJS} plottoa_vers.o

plottoa_vers.c: ${DOVERS} ${PLOTTOA_OBJS}
	@rm -f $@
	vers_string -c plottoa > $@

#
# Building debug
#
debug: debug.o
	cc -o debug debug.o


#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCTS) $(DERIVED) \
		$(GARBAGE)

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS):
	mkdirs $@

depend Makedep: $(DERIVED)
	rm -f Makedep
	$(CC) -M $(CDEFS) $(CFILES) $(SFILES) $(DERIVED) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
