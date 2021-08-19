#
# this file gets executed by every shell you start
#

# make sure the path is correct
set path=(. ~/Unix/bin /usr/local/bin /usr/ucb /bin /usr/bin /usr/sybase/bin ~/Apps /LocalApps /NextApps /NextAdmin /NextDeveloper/Demos)

# setup umask to something reasonable
umask 022

# make the prompt palatable
if( ${?prompt} ) then
	set host=`hostname`
	set prompt="${host}> "
	set history=50
	# put aliases and other things down here
	alias ls ls -F
	alias mail /usr/ucb/Mail
endif
