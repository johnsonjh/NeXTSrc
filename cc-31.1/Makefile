# This Makefile is derived from the standard parameterization
# file for describing one or more products in a single directory 
# and associated sub directories.
# Copyright 1988, NeXT, Inc.
#
# This defines the parameters of the make operation for
# the product defined in PRODUCT.  If PRODUCTS is defined
# to a list prod1 prod2 ... of products then the 
# parameterization of each of th0se products will be in 
# prod1.mk, prod2.mk, ...  If SUBDIRS is defined then those
# subdirectories will be "made" also.

# (CHOOSE ONE OF THESE)
# Name the product or list of products.  This is the name under
# which this file will be installed.  If a list of products
# is name (e.g. prod1 prod2 prod3) then the '.mk' files must
# be present (e.g. prod1.mk prod1.mk prod3.mk)

PRODUCT =
PRODUCTS =

# Name any subdirectories which should be also respond to
# install, installsrc, local, and clean.  These are done
# depth first.

SUBDIRS = as bison cc objc++ ld gprof misc include mkshlib man notes RelNotes
INSTALLSUBDIRS = g++

# The rest of this files is for specification of the components
# of the SINGLE PRODUCT named above.

# Name the place where the product will be installed.

INSTALLDIR = /

# If the product is formed from a csh or sh script, name
# the script (Choose just one.)

CSHFILE =
SHFILE =

# Alternatively, name the .m, .c, .psw, .pswm, .s, .l, .lm, 
# .y, .ym, and .ps for the product.  All of these generate 
# .o's for the product by the rule impield by the suffix.

MFILES =
CFILES =
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

LOADLIBS =

# Name the header files which the user will see in the
# released product and the header install directory.

USER_HFILES =
HINSTALLDIR = /

# Add the names of files which are part of the source but are not
# normally released with the product.  This includes all 
# text files picked up from this directory automatically in 
# the compiles (such as header files), makefiles, and anything
# else necessary to complete the list of sources.

OTHER_TEXT_FILES = Makefile Makedep

# Name the .spec and .man files which are a part of this product.
# For .spec files, the order of the class heirarchy is important
# and must be maintained here.

SPECFILES =
SPECINSTALLDIR =
MANFILES =
MANINSTALLDIR = /usr/man

# These actions will be done first when building the .NEW product.
# If this is unnecessary leave it blank, but don't delete this.

$(PRODUCT).NEWfirst:

# You may override the default rules here (e.g. .c.o:)

# The rest of the work is done by the standard common makefile
MAKEINCDIR = /usr/local/lib/Makefiles
include $(MAKEINCDIR)/Makeinc.common

installsrc: SRCROOT
	tar cf - Makefile $(SUBDIRS) $(INSTALLSUBDIRS) | (cd ${SRCROOT}; tar xfp -)

installhdrs: DSTROOT
	cd include; $(MAKE) DSTROOT=$(DSTROOT) install

installdoc: DSTROOT
	cd man;   $(MAKE) DSTROOT=$(DSTROOT) install
	cd notes; $(MAKE) DSTROOT=$(DSTROOT) install

SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi
