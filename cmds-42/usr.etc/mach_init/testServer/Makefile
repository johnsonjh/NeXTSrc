CFLAGS = -g -I..

all: testServer listServer

testServer: testServer.o bootstrapUser.o
	cc -o testServer testServer.o bootstrapUser.o

listServer: listServer.o bootstrapUser.o
	cc -o listServer listServer.o bootstrapUser.o


bootstrapUser.o: ../bootstrapUser.c
	cc -c -g ../bootstrapUser.c

clean:
	rm -f *.o testServer listServer
