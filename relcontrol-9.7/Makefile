CFLAGS = -O

LIB = libVersion.a
LIB_DIR = usr/local/lib
INSTALLFLAGS_LIB = -c -g wheel

HFILES = NXVersion.h
INCLUDE_DIR = $(DSTDIR)/usr/local/include/relcontrol
INSTALLFLAGS_HDR = -c -m 644 -g wheel

DATA = software_version
DATA_DIR = private/adm
INSTALLFLAGS_DATA = -c -m 444 -o root -g wheel

USER_BOM = relcontrol.user_bom
USER_BOM_DIR = usr/local/BOMs
INSTALLFLAGS_USER_BOM = -c -m 444 -o root -g wheel

CFILES = NXVersion.c
SRCFILES = $(HFILES) $(CFILES) $(DATA) $(USER_BOM)

OBJS = NXVersion.o

all: $(LIB) test_lib

$(LIB): $(OBJS)
	ar cr $(LIB) $(OBJS)
	ranlib $(LIB)
		
test_lib: test.o $(LIB)
	cc -o test_lib test.o $(LIB)
	echo testing ...
	test_lib

$(OBJS): $(HDRS)

install: $(LIB) $(DSTROOT)/$(LIB_DIR) $(DSTROOT)/$(INCLUDE_DIR) $(DSTROOT)/$(DATA_DIR) $(DSTROOT)/$(USER_BOM_DIR)
	install $(INSTALLFLAGS_LIB) $(LIB) $(DSTROOT)/$(LIB_DIR)
	install $(INSTALLFLAGS_HDR) $(HFILES) $(DSTROOT)/$(INCLUDE_DIR)
	install $(INSTALLFLAGS_DATA) $(DATA) $(DSTROOT)/$(DATA_DIR)
	install $(INSTALLFLAGS_USER_BOM) $(USER_BOM) $(DSTROOT)/$(USER_BOM_DIR)

$(DSTROOT)/$(LIB_DIR) $(DSTROOT)/$(INCLUDE_DIR) $(DSTROOT)/$(DATA_DIR) $(DSTROOT)/$(USER_BOM_DIR) $(SRCROOT):
	mkdirs $@
	
installsrc:  SRCROOT $(SRCROOT)
	cp Makefile $(SRCFILES) $(SRCROOT)

SRCROOT:
	@if [ -n "${$@}" ]; \
		then \
			exit 0; \
		else \
			echo Must define $@; \
			exit 1; \
		fi

clean:
	/bin/rm -f *.o $(LIB) test_lib


