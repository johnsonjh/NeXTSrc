###########################################################
#
#	Sendmail configuration file for NeXT machines who share
#	their /usr/spool/mail directory with a central mail server.
#
#	You should not need to edit this file to customize it for your
#	network configuration unless you need to change the name of
#	the mailhost.
#
#	See the paper "Sendmail Installation and Administration Guide"
#	for more information on the format of this file.
#
#

# local UUCP connections -- not forwarded to mailhost
# The local UUCP connections are output by the uuname program.
FV|/usr/bin/uuname

# my official hostname
Dj$?m $w.$m $| $w $.

# major relay mailer
DMetherl

# major relay host
DRmailhost
CRmailhost

include(nextbase.m4)

include(etherlm.m4)

include(uucpm.m4)

include(zerobase.m4)

################################################
###  Machine dependent part of ruleset zero  ###
################################################

# resolve names we can handle locally
R<@$=V.uucp>:$+		$:$>9 $1			First clean up, then...
R<@$=V.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$=V.uucp>		$#uucp  $@$2 $:$1		user@host.uucp

# non-local UUCP hosts get kicked upstairs
R$+<@$+.uucp>		$#$M  $@$R $:$2!$1

# optimize names of known ethernet hosts
R$*<@$+.LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	user@host.here
# hosts in no domains are assumed to be local
R$*<@$->$*		$#ether $@$2 $:$1<@$2>$3	user@host

# other non-local names will be kicked upstairs
R$+			$:$>9 $1			Clean up, keep <>
R$*<@$+>$*		$#$M    $@$R $:$1<@$2>$3	user@some.where
R$*@$*			$#$M    $@$R $:$1<@$2>		strangeness with @

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name which gets delivered to mailhost
R$+			$#$M    $@$R $:$1		local names
