#!/bin/sh

answer=`alert "Do you really want to remove the ICH_AC97 driver now?" "No" "Yes"`
if [ $answer == "No" ]
then
	alert "ICH AC97 driver NOT uninstalled!" "OK"
	exit
fi

if [ -e ~/config/add-ons/kernel/drivers/dev/audio/old/ich_ac97 ]
then
    rm ~/config/add-ons/kernel/drivers/dev/audio/old/ich_ac97
fi

if [ -e ~/config/add-ons/kernel/drivers/dev/audio/multi/ich_ac97 ]
then
    rm ~/config/add-ons/kernel/drivers/dev/audio/multi/ich_ac97
fi

if [ -e ~/config/add-ons/kernel/drivers/bin/ich_ac97 ]
then
    rm ~/config/add-ons/kernel/drivers/bin/ich_ac97
fi

if [ -e /boot/beos/system/add-ons/kernel/drivers/bin/i801.zip ]
then
    cd /boot/beos/system/add-ons/kernel/drivers/bin
    unzip i801.zip  
    rm i801.zip
    cd `dirname "$0"`
fi

alert "ICH AC97 driver removed! Restart media services to stop it if it's still running." "OK"
