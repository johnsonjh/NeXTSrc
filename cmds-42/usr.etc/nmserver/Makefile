all: FRC
	cd build; make default
	cd build; make utilprog
	
install: all
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	cd build; make install DSTROOT=$$DSTROOT

clean:
	cd build; make clean

FRC:
