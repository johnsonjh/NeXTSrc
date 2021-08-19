# @(#)suucpm.m4	1.1 87/09/08 3.2/4.3NFSSRC
############################################################
############################################################
#####
#####		Smart UUCP Mailer specification
#####
#####	The other end must speak domain-based  addresses for
#####	this to work.
#####
#####		@(#)suucpm.m4 1.5 86/07/16 SMI; from UCB 3.1 5/1/83
#####
############################################################
############################################################

Msuucp,	P=/usr/bin/uux, F=sDFMhuU, S=15, R=15
	A=uux - -r $h!rmail ($u)

S15
R$*<@$+>$*		$@$1<@$2>$3			accept usual domain name
R$+			$:$1<@LOCAL>			stick on our host name
