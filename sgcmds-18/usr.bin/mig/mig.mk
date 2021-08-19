PRODUCT = mig

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

SHFILE = mig.sh

# Name the header files which the user will see in the
# released product and the header install directory.
#
# This file installed by the kernel project.
# USER_HFILES = mig_errors.h
# HINSTALLDIR = /usr/include

# These actions will be done first when building the .NEW product.
# If this is unnecessary leave it blank, but don't delete this.

$(PRODUCT).NEWfirst:

# These parameters affect the behavior of commands within the include
# file.  These are for install, speculate, cc, and ld.

IFLAGS = -c
SPECFLAGS =
CFLAGS = -O -MD
LDFLAGS =

# The rest of the work is done by the standard common makefile
MAKEINCDIR = /usr/local/lib/Makefiles
include $(MAKEINCDIR)/Makeinc.common

# Suffix rules provided by 'make' or by the above include file
# may be overriden here (e.g. .c.o:)

