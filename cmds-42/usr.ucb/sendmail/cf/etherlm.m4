############################################################
#####
#####		Local Ethernet Mailer specification
#####
#####	This mailer acts as the local mailer for those machines
#####	that have a remotely mounted mail partition.

Metherl,P=[TCP], F=msDFuCXN, S=12, R=22, A=TCP $h
S12
# None needed.

S22
R$*<@LOCAL>		$@$1			remove ugly old local
