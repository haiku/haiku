#!/bin/sh
base=`dirname "$0"`
cd "$base"

RETURN=`alert "There can only be ONE version of input_server on the system at one time (see the enclosed README file for why).

Choose 'Backup' if you wish to keep your old input_server, Keymap preferences app, Keyboard preferences app, Mouse preferences app, keymap app and this is the first time you're installing the InputKit Replacement.  Otherwise, you should choose 'Purge' to clear the other versions from your system." "Purge" "Backup" "Abort - Don't do anything!"`

if [[ $RETURN = Purge ]]
then
	# note: we don't remove libmail.so, because it doesn't matter here, and there may be symlinks and things
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-input_server"' | grep /boot/beos | xargs rm -f
elif [[ $RETURN = Backup ]]
then
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-input_server"' | grep /boot/beos | xargs zip -ym /boot/home/inputkit.zip
else
	alert "No backup will be done.  That means it's up to YOU to purge all of your old input_server and ensure that the new version is the only version."

	exit -1
fi

if [ -n "$TTY" ]
then
    unzip -d / install.zip
else
    response=`alert "Would you like to automatically overwrite existing files, or receive a prompt?" "Overwrite" "Prompt"`
    if [ $response == "Overwrite" ]
    then
        unzip -od / install.zip
        alert "Finished installing" "Thanks"
    else
        if [ -e /boot/beos/apps/Terminal ]
        then
            terminal=/boot/beos/apps/Terminal
        else
            terminal=`query Terminal | head -1`
        fi
        $terminal -t "installer" /bin/sh "$0"
    fi
fi
