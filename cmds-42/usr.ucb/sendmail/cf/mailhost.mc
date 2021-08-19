############################################################
#
#	Sendmail configuration file for "MAIN MACHINES"
#
#	You should install this file as /etc/sendmail/sendmail.cf
#	if your machine is the main (or only) mail-relaying
#	machine in your domain.  Then edit the file to
#	customize it for your network configuration.
#
#	See the paper "Sendmail Installation and Administration Guide"
#	for more information on the format of this file.
#
#	@(#)main.mc 1.0 88/10/26 NeXT; from SMI 3.2/4.3 NFSSRC
#

###	local info

# my official hostname
Dj$?m $w.$m $| $w $.
# If you want the gateway machine to appear to be INSIDE the domain, use:
#Dj$w.$m

# Major relay mailer - typical choice is "ddn" if you are on the
# Defense Data Network (e.g. Arpanet or Milnet).  All mail for
# non-local domains will be forwarded to the major relay host using
# the major relay mailer.  We chose "uucp" as the default because most
# isolated networks use it to link their network with the outside
# world.
#
# If you want to use a relay mailer, examine ruleset 0.  There are some
# rules that need to be uncommented
DMuucp

# major relay host: use the $M mailer to send mail to other domains
# To have mail automatically forwarded to other domains, you should
# replace this with the name of your major relay host.
ifdef(`m4GATEWAY',, `define(m4GATEWAY,mail-relay)')dnl
DR m4GATEWAY
CR m4GATEWAY

# local UUCP connections - output from the uuname command
FV|/usr/bin/uuname

# options that you probably want on a mailhost:

# checkpoint the queue after this many receipients
OC10

# refuse to send tiny messages to more than these recipients
Ob10

include(nextbase.m4)

include(uucpm.m4)

include(ddnm.m4)

############################################################
#
#		RULESET ZERO
#
#	This is the ruleset that determines which mailer a name goes to.

# Ruleset 30 just calls rulesets 3 then 0.
S30
R$*			$: $>3 $1			First canonicalize
R$*			$@ $>0 $1			Then rerun ruleset 0

S0
# On entry, the address has been canonicalized and focused by ruleset 3.
# Handle special cases.....
R@			$#local $:$n			handle <> form
# For numeric spec, you can't pass spec on to receiver, since rcvr's
# are not smart enough to know that [x.y.z.a] is their own name.
R<@[$+]>:$*		$:$>9 <@[$1]>:$2		Clean it up, then...
R<@[$+]>:$*		$#ether $@[$1] $:$2		numeric internet spec
R<@[$+]>,$*		$#ether $@[$1] $:$2		numeric internet spec
R$*<@[$+]>		$#ether $@[$2] $:$1		numeric internet spec

# resolve the local hostname to "LOCAL".
R$*<$*$=w.LOCAL>$*	$1<$2LOCAL>$4			thishost.LOCAL
R$*<$*$=w.uucp>$*	$1<$2LOCAL>$4			thishost.uucp
R$*<$*$=w>$*		$1<$2LOCAL>$4			thishost

# Mail addressed explicitly to the domain gateway (us)
R$*<@LOCAL>		$@$>30$1			strip our name, retry
R<@LOCAL>:$+		$@$>30$1			retry after route strip

# deliver to known ethernet hosts explicitly specified in our domain
R$*<@$+.LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	user@host.ourdomain
# hosts in no domains are assumed to be local
R$*<@$->$*		$#ether $@$2 $:$1<@$2>$3	user@host

# Clean up addresses for external use -- kills LOCAL, route-addr ,=>: 
R$*			$:$>9 $1			Then continue...

# resolve UUCP domain
R<@$=V.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$=V.uucp>		$#uucp  $@$2 $:$1		user@host.uucp
R<@$-.uucp>:$+		$#error $:Never heard of UUCP host $1
R$+<@$-.uucp>		$#error $:Never heard of UUCP host $2

# If you want to pass all other explicit domain names up the ladder
# to our forwarder then uncomment the following line.
#R$*<@$*.$+>$*		$#$M    $@$R $:$1<@$2.$3>$4	user@any.domain
# and comment out this one.
R$*<@$+.$->$*		$#ddn $@ $2.$3 $:$1<@$2.$3>$4	user@any.domain

# All addresses in the rules ABOVE are absolute (fully qualified domains).
# Addresses BELOW can be partially qualified.

R$*@$*			$#error $:I don't understand $1@$2

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name
R$+			$#local $:$1			local names
