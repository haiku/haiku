#!/bin/sh

answer=`alert "Do you really want to uninstall the Sis 7018 driver?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

rm ~/config/add-ons/kernel/drivers/bin/sis7018
rm ~/config/settings/kernel/drivers/sis7018
rm ~/config/add-ons/kernel/drivers/dev/audio/old/sis7018

alert "The SiS7018 driver has been removed.

Restart your system now."
