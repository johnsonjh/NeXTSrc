MORE=atan2.o cabs.o cbrt.o sqrt.o sincos.o tan.o argred.o support.o infnan.o
MV= mv -f

.s.o:
####	insert additional code for profiling
	sed -f mcount.sed $*.s | ${AS} -o $*.o
	-ld -X -r $*.o
	${MV} a.out ../profiled/$*.o
	${AS} -o $*.o $*.s
	-ld -x -r $*.o
	${MV} a.out $*.o

all: ../libm.a ../libm_p.a

../libm_p.a ../libm.a: ${MORE}
	cd ../profiled; ar cru ../libm_p.a ${MORE}
	ar cru ../libm.a ${MORE}
