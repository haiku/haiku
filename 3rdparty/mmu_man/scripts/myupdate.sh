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
	#notice "Checking for updates..."
	pkgman update || exit $?
	sync
	if [ "$(ls /system/packages/*_hrev*-* | grep -v $(uname -v | cut -d' ' -f1))" = "" ]; then
		open /system/Deskbar < /dev/null > /dev/null 2>&1 &
		notice "Restarting Deskbar..."
		notice "Nothing to do"
		read
		exit 0
	fi
	makebootable /boot
	sync
	cat > $(finddir B_USER_SETTINGS_DIRECTORY)/boot/launch/myupdate2.sh << EOF
#!/bin/sh
cd "$PWD"
$0 -2
EOF
	chmod +x $(finddir B_USER_SETTINGS_DIRECTORY)/boot/launch/myupdate2.sh
	sync
	notice "Restarting Deskbar..."
	/system/Deskbar < /dev/null > /dev/null 2>&1 &
	notice "You should reboot now..."
	shutdown -r -a
	read
	exit 0
fi

rm $(finddir B_USER_SETTINGS_DIRECTORY)/boot/launch/myupdate2.sh

disdir="$(finddir B_SYSTEM_PACKAGES_DIRECTORY)/disabled"
mkdir -p "$disdir"

notice "Moving old packages to disabled/ ..."
mv $(ls /system/packages/*_hrev*-* | grep -v $(uname -v | cut -d' ' -f1)) $disdir/

sync

#notice "Adding back deskbar addons..."
#ProcessController -deskbar
#NetworkStatus --deskbar

notice "Done"

read
