
#
#  NeXT include file for Internal Application Makefile
#  Copyright 1987, NeXT, Inc.
#
#  This Makefile is included to gain the rules for making and installing
#  an application.
#

# a little doc for when the "help" target is made
.DEFAULT help:
	@if (test "$<");		\
	then echo 'ERROR: Unrecognized Makefile target "$<"';	\
	fi
	@echo '  install-  to install the application';\
	echo '  installsrc-  to install the source of the application';\
	echo '  full_install-  to install the application and source';\
	echo '  $(NAME)-  to make the local copy of the application';\
	echo '  clean-    to remove all files but the source';\
	echo '  print-    to print out all the source files';\
	echo '  wc-       to get the line, word, and byte count of the source';\
	echo '  size-     to get the size of all object files';\
	echo '  diff-     diffs current source against installed source';\
	echo '  tags-     to run ctags on the source';\
	echo '  depend-   to update Makefile dependencies on include files';\
	echo '  lint-     to lint all the source'

# this is the fruit of this project
PRODUCT = $(NAME)

# flags passed to vers_string
VERSFLAGS = -c

# flags for install
BINIFLAGS = 

# making "all" causes the library to be built
all:: $(PRODUCT)

include $(MAKE_DIR)/Makefile.common

# an application is made by first making all its components, and then
# linking the whole thing together.
$(PRODUCT): $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PRODUCT) $(OFILES) $(LOADLIBES)

# creates products and installs them, and installs the source.
full_install: install installsrc

# creates products and installs them.  Does NOT install source.
install:: force_version $(PRODUCT) $(DSTROOT) inst_product additional_installs

# installs the stripped binary
inst_product: $(BINDIR) $(PRODUCT)
	install $(IFLAGS) $(BINIFLAGS) -m 555 $(PRODUCT) $(BINDIR)
	
# install the source
installsrc::
	mkdirs $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(INSTALL_FILES) | (cd $(SRCROOT); tar xf -)
	chmod 444 $(SRCROOT)/*
#	TEMP_VAR1=`pwd`;				\
#	TEMP_VAR2=`basename $$TEMP_VAR1`;		\
#	mkdirs -m 755 $(SRCDIR)/$$TEMP_VAR2;		\
#	for f in $(INSTALL_FILES);			\
#		do (cp $$f $(SRCDIR)/$$TEMP_VAR2;	\
#		chmod 444 $(SRCDIR)/$$TEMP_VAR2/$$f)	\
#	done

# makes sure this directory exists
$(BINDIR):
	mkdirs -m 755 $@

# shows object code size
size: $(OFILES)
	@/bin/size $(OFILES) | awk '{ print; for( i = 0; ++i <= 4;) x[i] += $$i } \
	 END { print x[1] "\t" x[2] "\t" x[3] "\t" x[4] }'
