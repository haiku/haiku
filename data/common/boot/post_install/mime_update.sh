#!/bin/sh

_progress () {
	notify --type progress --group "MIME type updater" \
	--timeout ${3:-30} \
	--icon /boot/system/apps/DiskProbe \
	--messageID $0_$$ \
	--title "Updating file MIME types..." \
	--progress $1 "$2" >/dev/null
}

_progress 0.0 "desktop files"

# Make sure files on the desktop are mimeset first

for f in $(/bin/finddir B_DESKTOP_DIRECTORY 2>/dev/null\
	|| echo "/boot/home/Desktop")/*; do
	if [ -f "$f" ]; then
		mimeset -f "$f"
	fi
done

# Make sure all apps have a MIME DB entry.

SYSTEM=$(/bin/finddir B_SYSTEM_DIRECTORY 2>/dev/null || echo "/boot/system")

mimeset -f "$SYSTEM/bin/userguide"
mimeset -f "$SYSTEM/bin/welcome"

_progress 0.1 "system applications"
mimeset -apps -f "$SYSTEM/apps"
_progress 0.2 "documentation"
mimeset       -f "$SYSTEM/documentation"
_progress 0.3 "preferences"
mimeset -apps -f "$SYSTEM/preferences"
_progress 0.4 "servers"
mimeset -apps -f "$SYSTEM/servers"
_progress 0.5 "applications"
mimeset -apps -f "/boot/apps"
_progress 0.7 "application (by signature)"

query -f 'BEOS:APP_SIG=*' | xargs --no-run-if-empty mimeset -apps -f

_progress 1.0 "done" 10
