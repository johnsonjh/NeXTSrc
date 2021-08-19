stty erase "^?" intr "^C" kill "^U" -decctlq
echo "erase ^? intr ^C kill ^U"
PATH=.:/usr/ucb:/bin:/usr/bin:/usr/sybase/bin:${HOME}/Apps:/LocalApps:/NextApps:/NextAdmin:/NextDeveloper/Demos
TERM=`tset - -Q`
export PATH TERM HOME
umask 022
