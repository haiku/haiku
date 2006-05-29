#!/bin/sh

DRIVER_TITLE="USBVision"
DRIVER_NAME=usbvision

answer=`alert "Do you really want to uninstall the $DRIVER_TITLE driver?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

rm ~/config/add-ons/kernel/drivers/bin/$DRIVER_NAME
rm ~/config/settings/kernel/drivers/$DRIVER_NAME
rm ~/config/add-ons/kernel/drivers/dev/video/$DRIVER_NAME

rescan $DRIVER_NAME

alert "The $DRIVER_TITLE driver has been removed from your system."
