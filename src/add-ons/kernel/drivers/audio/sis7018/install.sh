#!/bin/sh

answer=`alert "Do you really want to install the SiS 7018 driver?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/beos/bin/install -d ~/config/add-ons/kernel/drivers/bin/
/boot/beos/bin/install -d ~/config/settings/kernel/drivers/
/boot/beos/bin/install -d ~/config/add-ons/kernel/drivers/dev/audio/old/
/boot/beos/bin/install `dirname $0`/sis7018 ~/config/add-ons/kernel/drivers/bin/
/boot/beos/bin/cp `dirname $0`/sis7018.settings.sample ~/config/settings/kernel/drivers/sis7018

mimeset ~/config/add-ons/kernel/drivers/bin/sis7018
mimeset ~/config/settings/kernel/drivers/sis7018

ln -sf ../../../bin/sis7018 ~/config/add-ons/kernel/drivers/dev/audio/old/sis7018

alert "The SiS 7018 driver is installed in your system.

Go to Preferences->Media and restart media services to activate driver."
