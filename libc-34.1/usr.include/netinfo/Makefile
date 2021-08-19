#
# Build netinfo header files from RPCGEN files
#
RPCGEN= rpcgen

all: nibind_prot.h ni_prot.h

nibind_prot.h: nibind_prot.x
	$(RPCGEN) -h $? -o $@

ni_prot.h: ni_prot.x
	$(RPCGEN) -h $? -o $@
