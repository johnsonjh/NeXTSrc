CFLAGS= -DNeXT_MOD
MORE=atan2.o cbrt.o trig.o cabs.o support.o fmod.o sqrt.o
MV= mv -f

.c.o:
	${CC} -pg ${CFLAGS} -c $*.c
	-ld -X -r $*.o
	${MV} a.out ../profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	${MV} a.out $*.o

all: ../libm.a ../libm_p.a

../libm.a ../libm_p.a: ${MORE}
	cd ../profiled; ar cru ../libm_p.a ${MORE}
	ar cru ../libm.a ${MORE}
