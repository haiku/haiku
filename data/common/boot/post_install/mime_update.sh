#!/bin/sh

# Make sure files on the desktop are mimeset first

for f in $(/bin/finddir B_DESKTOP_DIRECTORY 2>/dev/null\
	|| echo "/boot/home/Desktop")/*; do
	if [ -f $f ]; then
		mimeset -f $f
	fi
done

# Make sure all apps have a MIME DB entry.

SYSTEM=$(/bin/finddir B_SYSTEM_DIRECTORY 2>/dev/null || echo "/boot/system")

mimeset -apps -f "$SYSTEM/apps"
mimeset       -f "$SYSTEM/documentation"
mimeset -apps -f "$SYSTEM/preferences"
mimeset -apps -f "$SYSTEM/servers"
mimeset -apps -f "/boot/apps"

query -f 'BEOS:APP_SIG=*' | xargs --no-run-if-empty mimeset -apps -f
