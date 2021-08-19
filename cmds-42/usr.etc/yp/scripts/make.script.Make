#
# @(#)make.script	1.2 87/08/25 3.2/4.3NFSSRC
# @(#)make.script	1.2 86/11/21 NFSSRC
#	@(#)make.script 1.1 86/09/25 SMI
#PROGRAM
#
DIR =/etc
DOM = `domainname`
NOPUSH = ""
ALIASES = /etc/sendmail/aliases
YPPUSH = /usr/etc/yp/yppush
MAKEDBM = /usr/etc/yp/makedbm
REVNETGROUP = /usr/etc/yp/revnetgroup
STDHOSTS = /usr/etc/yp/stdhosts

all: passwd group hosts ethers networks rpc services protocols \
	netgroup aliases

passwd.time: $(DIR)/passwd
	-@if [ -f $(DIR)/passwd ]; then \
		awk 'BEGIN { FS=":"; OFS="\t"; } /^[a-zA-Z0-9_]/ { print $$1, $$0 }' \
		  $(DIR)/passwd  | $(MAKEDBM) - /etc/yp/$(DOM)/passwd.byname; \
		awk 'BEGIN { FS=":"; OFS="\t"; } /^[a-zA-Z0-9_]/ { print $$3, $$0 }' \
		  $(DIR)/passwd | $(MAKEDBM) - /etc/yp/$(DOM)/passwd.byuid; \
		touch passwd.time; \
		echo "updated passwd"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) passwd.byname; \
			$(YPPUSH) passwd.byuid; \
			echo "pushed passwd"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/passwd"; \
	fi

group.time: $(DIR)/group
	-@if [ -f $(DIR)/group ]; then \
		awk 'BEGIN { FS=":"; OFS="\t"; } { print $$1, $$0 }' \
		    $(DIR)/group | $(MAKEDBM) - /etc/yp/$(DOM)/group.byname; \
		awk 'BEGIN { FS=":"; OFS="\t"; } { print $$3, $$0 }' \
		    $(DIR)/group | $(MAKEDBM) - /etc/yp/$(DOM)/group.bygid; \
		touch group.time; \
		echo "updated group"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) group.byname; \
			$(YPPUSH) group.bygid; \
			echo "pushed group"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/group"; \
	fi

hosts.time: $(DIR)/hosts
	-@if [ -f $(DIR)/hosts ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/hosts | $(STDHOSTS) \
		    | awk '{for (i = 2; i <= NF; i++) print $$i, $$0}' \
		    | $(MAKEDBM) - /etc/yp/$(DOM)/hosts.byname; \
		$(STDHOSTS) $(DIR)/hosts | \
		    awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$1, $$0 }' \
		    | $(MAKEDBM) - /etc/yp/$(DOM)/hosts.byaddr; \
		touch hosts.time; \
		echo "updated hosts"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) hosts.byname; \
			$(YPPUSH) hosts.byaddr; \
			echo "pushed hosts"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/hosts"; \
	fi

ethers.time: $(DIR)/ethers
	-@if [ -f $(DIR)/ethers ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/ethers \
		| awk 'BEGIN { X["A"]="a"; X["B"]="b"; X["C"]="c"; X["D"]="d";X["E"]="e"; X["F"]="f"; } \
		    { for (i = 1; i <= length($$1); i++) \
		      if (substr($$1, i, 1) >= "A" && substr($$1, i, 1) <= "Z") \
			printf("%s", X[substr($$1, i, 1)]) ; \
		      else \
			printf("%s", substr($$1, i, 1)); printf(" "); \
		    print $$2}' \
		|awk '{print $$1, $$0; for (i = 3;i <= NF;i++) print $$i,$$0}' \
		| $(MAKEDBM) - /etc/yp/$(DOM)/ethers.byaddr; \
		awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		   $(DIR)/ethers | \
		$(MAKEDBM) - /etc/yp/$(DOM)/ethers.byname;\
		touch ethers.time; \
		echo "updated ethers"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) ethers.byname; \
			$(YPPUSH) ethers.byaddr; \
			echo "pushed ethers"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/ethers"; \
	fi

networks.time: $(DIR)/networks
	-@if [ -f $(DIR)/networks ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/networks | awk \
		    '{print $$1, $$0; for (i = 3;i <= NF;i++) print $$i,$$0}' \
		    | $(MAKEDBM) - /etc/yp/$(DOM)/networks.byname; \
		awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		   $(DIR)/networks | $(MAKEDBM) - /etc/yp/$(DOM)/networks.byaddr;\
		touch networks.time; \
		echo "updated networks"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) networks.byname; \
			$(YPPUSH) networks.byaddr; \
			echo "pushed networks"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/networks"; \
	fi

services.time: $(DIR)/services
	-@if [ -f $(DIR)/services ]; then \
		awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		    $(DIR)/services| $(MAKEDBM) - /etc/yp/$(DOM)/services.byname;\
		touch services.time; \
		echo "updated services"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) services.byname; \
			echo "pushed services"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/services"; \
	fi

rpc.time: $(DIR)/rpc
	-@if [ -f $(DIR)/rpc ]; then \
		awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		    $(DIR)/rpc| $(MAKEDBM) - /etc/yp/$(DOM)/rpc.bynumber;\
		touch rpc.time; \
		echo "updated rpc"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) rpc.bynumber; \
			echo "pushed rpc"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/rpc"; \
	fi

protocols.time: $(DIR)/protocols
	-@if [ -f $(DIR)/protocols ]; then \
		awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		    $(DIR)/protocols | $(MAKEDBM) - \
		    /etc/yp/$(DOM)/protocols.bynumber; \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/protocols | awk \
		    '{print $$1,$$0; for (i = 3;i <= NF;i++) print $$i, $$0}' \
		    | $(MAKEDBM) - /etc/yp/$(DOM)/protocols.byname; \
		touch protocols.time; \
		echo "updated protocols"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) protocols.byname; \
			$(YPPUSH) protocols.bynumber; \
			echo "pushed protocols"; \
			else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/protocols"; \
	fi

netgroup.time: $(DIR)/netgroup
	-@if [ -f $(DIR)/netgroup ]; then \
		$(MAKEDBM) $(DIR)/netgroup /etc/yp/$(DOM)/netgroup; \
		$(REVNETGROUP) -u < $(DIR)/netgroup \
			| $(MAKEDBM) - /etc/yp/$(DOM)/netgroup.byuser; \
		$(REVNETGROUP) -h < $(DIR)/netgroup \
			| $(MAKEDBM) - /etc/yp/$(DOM)/netgroup.byhost; \
		touch netgroup.time; \
		echo "updated netgroup"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) netgroup; \
			$(YPPUSH) netgroup.byuser; \
			$(YPPUSH) netgroup.byhost; \
			echo "pushed netgroup"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(DIR)/netgroup"; \
	fi

aliases.time: $(ALIASES)
	-@if [ -f $(ALIASES) ]; then \
		cp $(ALIASES) /etc/yp/$(DOM)/mail.aliases; \
		/usr/lib/sendmail -bi -oA/etc/yp/$(DOM)/mail.aliases; \
		rm /etc/yp/$(DOM)/mail.aliases; \
		touch aliases.time; \
		echo "updated aliases"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) mail.aliases; \
			echo "pushed aliases"; \
		else \
			: ; \
		fi \
	else \
		echo "couldn't find $(ALIASES)"; \
	fi

passwd: passwd.time
group: group.time
hosts: hosts.time
ethers: ethers.time
networks: networks.time
rpc: rpc.time
services: services.time
protocols: protocols.time
netgroup: netgroup.time
aliases: aliases.time
$(DIR)/passwd:
$(DIR)/group:
$(DIR)/hosts:
$(DIR)/ethers:
$(DIR)/networks:
$(DIR)/rpc:
$(DIR)/services:
$(DIR)/protocols:
$(DIR)/netgroup:
$(ALIASES):
