#
# This file gets executed once at login or window startup.
#
set noglob; eval `tset -Q -s`; unset noglob
set term=$TERM
stty decctlq intr "^C" erase "^?" kill "^U"
stty -tabs
cd
