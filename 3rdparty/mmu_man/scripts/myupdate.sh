#!/bin/sh

cd "$(dirname "$0")"

test "$1" = "-t" || exec Terminal -t "My Update" "$0" -t "$@" && shift

notice () {
	notify --type information \
		--icon /boot/system/apps/HaikuDepot \
		--title "Update" \
		"$@"
	echo "$@"
}

if [ "$1" != "-2" ]; then

	notice "Closing Deskbar..."
	notice "Checking for updates..."
	quit application/x-vnd.Be-TSKB
	sync
	hrev_current="$(uname -v | cut -d' ' -f1)"
	pkgman update
	pkgman_err=$?
	sync
	if [ $pkgman_err -gt 0 -o "$(ls /system/packages/*_hrev*-* | grep -v $hrev_current)" = "" ]; then
		open /system/Deskbar < /dev/null > /dev/null 2>&1 &
		notice "Restarting Deskbar..."
		#notice "Nothing to do"
		read -p "Press a key..."
		exit $pkgman_err
	fi
	makebootable /boot
	sync
	notice "Restarting Deskbar..."
	/system/Deskbar < /dev/null > /dev/null 2>&1 &
	notice "You should reboot now..."
	shutdown -r -a
	sleep 5
	#read
	exit 0
fi

rm $(finddir B_USER_SETTINGS_DIRECTORY)/boot/launch/myupdate2.sh

