#!/bin/sh

answer=`alert "

Please read the readme file first!


It contains recovery instructions, which you will need if your computer crashes, freezes or fails to start.


Do you really want to install the ICH AC97 driver now?" "No" "Yes"`


if [ $answer == "No" ]
then
	alert "ICH AC97 driver NOT installed!" "OK"
	exit
fi

if [ -e /boot/beos/system/add-ons/kernel/drivers/bin/i801 ]
then
    answer=`alert "The default BeOS ICH AC97 driver (i801) needs to be disabled. 
It will be restored if you uninstall this driver using the uninstall.sh file." "Abort" "OK"`
    if [ $answer == "Abort" ]
    then
        alert "ICH AC97 driver NOT installed!" "OK"
        exit
    fi
    curdir=`dirname "$0"`
    if [ "$curdir" == "." ]
    then
        curdir=`pwd`
    fi
    cd /boot/beos/system/add-ons/kernel/drivers/bin
    zip i801.zip i801  
    rm i801
    cd $curdir
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

unzip -od / `dirname "$0"`/install.zip

kill -9 Media
kill -9 media_server
kill -9 media_addon_server
kill -9 audio_server
sleep 1

rescan ich_ac97
/boot/beos/system/servers/media_server & 
sleep 5

alert "
Installation Complete.

Please enable Realtime audio and
select 'AC97 (ICH)' as output and 'None In' as input. 

Restart the Media Services if required.
Read the readme file if you experience problems."

/boot/beos/preferences/Media &
