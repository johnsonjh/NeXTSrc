# @(#)nextbase.m4	0.8 88/10/19 NeXT
#################################################
#
#	General configuration information

# local domain name
#
# This is now set from the resolver configuration call.  If your
# Internet domain is different from the domain name you would like to
# have appear in your mail headers, edit them to be your mail domain
# name.
# DmPodunk.EDU

# known hosts in this domain are obtained from gethostbyname() call

include(base.m4)

#######################
#   Rewriting rules

# special local conversions
S6
R$*<@$*.uucp>$*		$@$1<@$2.uucp>$3		no change to UUCP hosts
R$*<@$+>$*		$:$1<@$[$2$]>$3			find canonical hostname
R$*<@$*$=m>$*		$1<@$2LOCAL>$4			convert local domain

include(localm.m4)
include(etherm.m4)
