PRODUCT = hostinfo
PRODUCTS =

# Name any subdirectories which should be also respond to
# install, installsrc, local, and clean.  These are done
# depth first.

SUBDIRS =

# The rest of this files is for specification of the components
# of the SINGLE PRODUCT named above.

# Name the place where the product will be installed.

INSTALLDIR = /usr/bin

# If the product is formed from a csh or sh script, name
# the script (Choose just one.)

CSHFILE =
SHFILE =

# Alternatively, name the .m, .c, .psw, .pswm, .s, .l, .lm, 
# .y, .ym, and .ps for the product.  All of these generate 
# .o's for the product by the rule impield by the suffix.

MFILES =
CFILES = hostinfo.c
PSWFILES =
PSWMFILES =
SFILES =
LFILES =
LMFILES =
YFILES =
YMFILES =
PSFILES =

# Also for linked executable products, name any additional
# libraries to use in linking.  These should be expressed
# in the form of -l options, i.e. libm.a is expressed as '-lm'.

# merged with libc now
LOADLIBS = -lsys_s

# Name any test files which must be installed as part of this
# product.

TEXTFILES =

# Name the header files which the user will see in the
# released product and the header install directory.

USER_HFILES =
HINSTALLDIR = /

# Name the .spec and .man files which are a part of this 
# product (SINGULAR!).
# For .spec files, the order of the class heirarchy is important
# and must be maintained here.

SPECFILES =
SPECINSTALLDIR =
MANFILES =
MANINSTALLDIR = /usr/man

# Intermediate files should be removed by "clean".  Add files here.

GARBAGE =

# These actions will be done first when building the .NEW product.
# If this is unnecessary leave it blank, but don't delete this.

$(PRODUCT).NEWfirst:

# These parameters affect the behavior of commands within the include
# file.  These are for install, speculate, cc, and ld.

IFLAGS = -s -c
SPECFLAGS =
CFLAGS = -O -MD
LDFLAGS= -object

# The rest of the work is done by the standard common makefile
MAKEINCDIR = /usr/local/lib/Makefiles
include $(MAKEINCDIR)/Makeinc.common

# Suffix rules provided by 'make' or by the above include file
# may be overriden here (e.g. .c.o:)

