#!/bin/sh
base=`dirname "$0"`
cd "$base"

function backup()
{
	BACKUP="uninstall.zip"
	
	rm -f "$BACKUP"

	# Backup BONE system files
	zip -yqr "$BACKUP" "/boot/beos/system/add-ons/kernel/network"
	zip -yq  "$BACKUP" \
		"/boot/beos/system/lib/libnet.so" \
		"/boot/beos/system/lib/libnetdev.so" \
		"/boot/beos/system/lib/libnetapi.so" \
		"/boot/beos/system/lib/libsocket.so" \
		"/boot/beos/system/lib/libbind.so" \
		"/boot/beos/system/lib/libbnetapi.so" \
		"/boot/beos/system/lib/libnetconfig.so" \
		"/boot/beos/bin/ifconfig" \
		"/boot/beos/bin/ping" \
		"/boot/beos/bin/dhclient" \
		"/boot/beos/bin/dhconfig" \
		"/boot/beos/bin/dhcp_client" \
		"/boot/beos/bin/dhcpd" \
		"/boot/beos/bin/dhcrelay" \
		"/boot/beos/bin/dnskeygen" \
		"/boot/beos/bin/dnsquery" \
		"/boot/beos/bin/ftp" \
		"/boot/beos/bin/ftpd" \
		"/boot/beos/bin/host" \
		"/boot/beos/bin/inetd" \
		"/boot/beos/bin/ipalias" \
		"/boot/beos/bin/irpd" \
		"/boot/beos/bin/named" \
		"/boot/beos/bin/pppconfig" \
		"/boot/beos/bin/route" \
		"/boot/beos/bin/tcpdump" \
		"/boot/beos/bin/tcptrace" \
		"/boot/beos/bin/telnet" \
		"/boot/beos/bin/telnetd" \
		"/boot/beos/bin/traceroute"

	zip -yqr "$BACKUP" "/boot/beos/system/add-ons/boneyard"
	zip -yq  "$BACKUP" \
		"/boot/beos/preferences/Boneyard" \
		"/boot/home/config/be/Preferences/Boneyard"

	# Backup network drivers
	for file in /boot/beos/system/add-ons/kernel/drivers/dev/net/*
	do
		f=`basename "$file"`
		zip -yq "$BACKUP"	"/boot/beos/system/add-ons/kernel/drivers/bin/$f" \
							"/boot/beos/system/add-ons/kernel/drivers/dev/net/$f"
	done
	for file in /boot/home/config/add-ons/kernel/drivers/dev/net/*
	do
		f=`basename "$file"`
		zip -yq "$BACKUP"	"/boot/home/config/add-ons/kernel/drivers/bin/$f" \
							"/boot/home/config/add-ons/kernel/drivers/dev/net/$f"
	done

	zip -yqr "$BACKUP" \
		"/boot/beos/system/add-ons/kernel/obos_network" \
		"/boot/home/config/add-ons/kernel/obos_network"
	zip -yq  "$BACKUP" \
		"/boot/home/config/lib/libnet.so" \
		"/boot/home/config/lib/libnetapi.so" \
		"/boot/home/config/lib/libbind.so" \
		"/boot/home/config/lib/libsocket.so" \
		"/boot/home/config/lib/libbind.so" \
		"/boot/home/config/bin/arp" \
		"/boot/home/config/bin/ifconfig" \
		"/boot/home/config/bin/ping" \
		"/boot/home/config/bin/ppp_up" \
		"/boot/home/config/bin/pppconfig" \
		"/boot/home/config/bin/route" \
		"/boot/home/config/bin/traceroute" \
		"/boot/beos/etc/networks" \
		"/boot/beos/etc/protocols" \
		"/boot/beos/etc/resolv.conf" \
		"/boot/beos/etc/services"
}



# Checking for BONE stack...
if [ -f /boot/beos/system/add-ons/kernel/network/bone_tcp -a \
     -c /dev/net/api ]
then
	BONE=1
fi

# Checking for Haiku stack...
if [ \( -f /boot/beos/system/add-ons/kernel/obos_network/core -o \
        -f /boot/home/config/add-ons/kernel/obos_network/core \) -a \
	    -c /dev/net/stack ]
then
	HAIKU_NETKIT=1
fi

response=`alert "Do you want to backup files before installing Haiku networking kit. \
				This will create an uninstall.zip file to restore back your system." \
				"Backup & Install" "Install" "Abort - Don't do anything!"`

if [ "$response" = "Backup & Install" ]
then
	echo "Backup in progress...";
	backup;
	response="Install"
elif [ "$response" = "Install" ]
then
	response=`alert "No backup will be done.
That means it's up to YOU to ensure that you can restore your system in previous configuration.
We suggest STRONGLY to do it right now before it's too late..." \
		"Install" "Abort"`
fi

if [ "$response" != "Install" ]
then
	echo "Aborting..."
	exit -1
fi

echo "Installing..."

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


