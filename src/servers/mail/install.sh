#!/bin/sh
base=`dirname "$0"`
cd "$base"

RETURN=`alert "There can only be ONE version of mail_daemon on the system at one time (see the enclosed README file for why).

Choose 'Backup' if you wish to keep your old mail_daemon, E-mail preferences app, and BeMail, and this is the first time you're installing the Mail Daemon Replacement.  Otherwise, you should choose 'Purge' to clear the other versions from your system." "Purge" "Backup" "Abort - Don't do anything!"`

if [[ $RETURN = Purge ]]
then
	# note: we don't remove libmail.so, because it doesn't matter here, and there may be symlinks and things
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer"' | grep -v "`/bin/pwd`" | xargs rm -f
elif [[ $RETURN = Backup ]]
then
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer" || name == libmail.so' | grep -v "`/bin/pwd`" | xargs zip -ym /boot/home/maildaemon.zip
else
	alert "No backup will be done.  That means it's up to YOU to purge all of your old mail_daemons and ensure that the new version is the only version."
	exit -1
fi

if [[ `uname -m` == BePC ]] && test ! -e ~/config/lib/libssl.so; then

RETURN=`alert "You don't seem to have OpenSSL installed, which the mail daemon requires." "Get OpenSSL" "I Don't Care"`

if [[ $RETURN = "Get OpenSSL" ]]
then
	NetPositive http://www.bebits.com/app/1020 &
fi

fi

if [ -n "$TTY" ]
then
    quit "application/x-vnd.Be-POST"
    quit "application/x-vnd.Be-TSKB"
    unzip -d / install.zip
    /boot/beos/system/Deskbar > /dev/null &
else
    response=`alert "Would you like to automatically overwrite existing files, or receive a prompt?" "Overwrite" "Prompt"`
    if [ $response == "Overwrite" ]
    then
        quit "application/x-vnd.Be-POST"
        quit "application/x-vnd.Be-TSKB"
        unzip -od / install.zip
        /boot/beos/system/Deskbar > /dev/null &
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
