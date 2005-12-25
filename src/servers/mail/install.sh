#!/bin/sh
base=`dirname "$0"`
cd "$base"

RETURN=`alert "There can only be ONE version of mail_daemon on the system at one time (see the enclosed README file for why).

Choose 'Backup' if you wish to keep your old mail_daemon, E-mail preferences app, and BeMail, and this is the first time you're installing the Mail Daemon Replacement (saves them in /boot/home/maildaemon.zip).  Otherwise, you should choose 'Purge' to clear the other versions from your system." "Purge" "Backup" "Abort - Don't do anything!"`

if [[ $RETURN = Purge ]]
then
	# note: we don't remove libmail.so, because it doesn't matter here, and there may be symlinks and things
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.spamdbm" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer"' | grep -v "`/bin/pwd`" | xargs rm -f
elif [[ $RETURN = Backup ]]
then
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.spamdbm" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer" || name == libmail.so' | grep -v "`/bin/pwd`" | xargs zip -ym /boot/home/maildaemon.zip
else
	alert "No backup or install done.  That means it's up to YOU to purge all of your old mail_daemons, install the new ones and ensure that the new version is the only version."
	exit -1
fi

# Do the shutdown before the alert, so that programs that take 10 seconds to terminate (spam server) have time while the alert is up.
echo "Shutting down deskbar and other programs that may have locked the mail library in use."
quit "application/x-vnd.agmsmith.AGMSBayesianSpamServer"
quit "application/x-vnd.agmsmith.spamdbm"
quit "application/x-vnd.Be-POST"
quit "application/x-vnd.Be-TSKB"

echo "Removing obsolete files - either we are using a new name or they moved to a new place."
rm -v /boot/home/config/add-ons/mail_daemon/inbound_filters/AGMSBayesianSpamFilter # Obsolete filter.
rm -v /boot/home/config/lib/libtextencoding.so # We're replacing this lib with one in a system folder.

response=`alert "Would you like to automatically overwrite existing files, or receive a prompt?" "Overwrite" "Prompt"`

if [ $response == "Prompt" ]
then
	if [ -e /boot/beos/apps/Terminal ]
    then
	    terminal=/boot/beos/apps/Terminal
    else
        terminal=`query Terminal | head -1`
	fi
	if [ -e /boot/beos/bin/unzip ]
	then
		uz=/boot/beos/bin/unzip
	else
		uz=`query unzip | head -1`
	fi
	$terminal -t "installer" $uz -d / install.zip
else #  Overwrite
	unzip -od / install.zip
fi


# Reset the relevant parts of the MIME database, now that the new programs are installed.
echo "Resetting the MIME database to refer to the new programs."
setmime -remove application/x-vnd.Be-POST; mimeset /system/servers/mail_daemon
setmime -remove application/x-vnd.Be-MAIL; mimeset /boot/beos/apps/BeMail
setmime -remove application/x-vnd.agmsmith.AGMSBayesianSpamServer
setmime -remove application/x-vnd.agmsmith.spamdbm ; mimeset ~/config/bin/spamdbm
setmime -set text/x-email -preferredApp /boot/beos/apps/BeMail -preferredAppSig application/x-vnd.Be-MAIL
setmime -set text/x-vnd.be-maildraft -preferredApp /boot/beos/apps/BeMail -preferredAppSig application/x-vnd.Be-MAIL
setmime -set text/x-partial-email -preferredApp /boot/beos/system/servers/mail_daemon -preferredAppSig application/x-vnd.Be-POST


#Restart the deskbar
/boot/beos/system/Deskbar > /dev/null &


#Sometimes the OpenSSL installer is dumb and doesn't create the requisite symlinks
if test ! -e ~/config/lib/libssl.so && test -e ~/config/lib/libssl.so.0.9.7; then
	ln -s ~/config/lib/libssl.so.0.9.7 ~/config/lib/libssl.so
	ln -s ~/config/lib/libcrypto.so.0.9.7 ~/config/lib/libcrypto.so
fi
# might as well create these links, as they're useful for compiling
if test ! -e /boot/develop/lib/x86/libssl.so && test -e ~/config/lib/libssl.so; then
	ln -s ~/config/lib/libssl.so /boot/develop/lib/x86/libssl.so
	ln -s ~/config/lib/libcrypto.so /boot/develop/lib/x86/libcrypto.so
fi
if test ! -e /boot/develop/headers/openssl && test -e ~/config/include/openssl; then
	ln -s ~/config/include/openssl/ /boot/develop/headers/openssl
fi


# Set up the spam database manager file types and sound file names, make spam indices.
spamdbm InstallThings


# Launch prefs if this is a new install of MDR (saving prefs starts the daemon).
if test !  -e "/boot/home/config/settings/Mail/chains";	then
	/boot/beos/preferences/E-mail &
else # Need to explicitly restart the daemon.
	/boot/beos/system/servers/mail_daemon > /dev/null &
fi

alert "Finished installing" "Thanks"
