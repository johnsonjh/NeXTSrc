#
# This Makefile handles installing all stuff in the build_files project.
#
# (c) NeXT, Inc. 1988
#

LIBDIR=	$(DSTROOT)/usr/local/lib
SRCROOT=$(DSTROOT)/src/build_files
MAKEFILE_DIR = $(LIBDIR)/Makefiles
ALLDIRS = $(LIBDIR) $(MAKEFILE_DIR)

MAKEFILE_FILES = Makefile.lib.user Makefile.lib Makefile.app \
		Makefile.app.user Makefile.common Makeinc.common

install: $(ALLDIRS)
	cd $(MAKEFILE_DIR); /bin/rm -f $(MAKEFILE_FILES)
	install -q -m 444 $(MAKEFILE_FILES) $(MAKEFILE_DIR)

clean:

# install the source
installsrc:
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	for f in $(MAKEFILE_FILES) Makefile;		\
		do (cp $$f $(SRCROOT); chmod 444 $(SRCROOT)/$$f)	\
	done

$(ALLDIRS):
	mkdirs $@

diff:
	for f in $(MAKEFILE_FILES);					\
	    do (echo $$f;						\
	    csh -f -c "diff -c $(MAKEFILE_DIR)/$$f $$f; exit 0")	\
	done
