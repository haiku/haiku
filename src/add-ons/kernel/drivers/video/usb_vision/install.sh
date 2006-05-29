#!/bin/sh

DRIVER_TITLE="USBVision"
DRIVER_NAME=usbvision

answer=`alert "Do you really want to install the $DRIVER_TITLE driver?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/beos/bin/install -d ~/config/add-ons/kernel/drivers/bin/
/boot/beos/bin/install -d ~/config/settings/kernel/drivers/
/boot/beos/bin/install -d ~/config/add-ons/kernel/drivers/dev/video/
/boot/beos/bin/install `dirname $0`/$DRIVER_NAME ~/config/add-ons/kernel/drivers/bin/
/boot/beos/bin/cp `dirname $0`/$DRIVER_NAME.settings.sample ~/config/settings/kernel/drivers/$DRIVER_NAME

mimeset ~/config/add-ons/kernel/drivers/bin/$DRIVER_NAME
mimeset ~/config/settings/kernel/drivers/$DRIVER_NAME

ln -sf ../../bin/$DRIVER_NAME ~/config/add-ons/kernel/drivers/dev/video/$DRIVER_NAME

rescan $DRIVER_NAME

alert "The $DRIVER_TITLE driver has been installed in your system."
