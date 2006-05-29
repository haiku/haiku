#!/bin/sh
##########################################################
##
##     USB Storage Module installation script
##
##  This file is a part of BeOS USB SCSI interface module project.
##  Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
##  All rights reserved.
##
##  $Source: /cvsroot/sis4be/usb_scsi/install.sh,v $
##  $Author: zharik $
##  $Revision: 1.5 $
##  $Date: 2004/06/06 09:19:47 $
##
##########################################################

MODULE_TITLE="USB Storage Module"
MODULE_NAME=usb
SETTINGS_NAME=usb_scsi

answer=`alert "Do you really want to install the $MODULE_TITLE ?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/beos/bin/install -d ~/config/add-ons/kernel/busses/scsi/
/boot/beos/bin/install `dirname $0`/$MODULE_NAME ~/config/add-ons/kernel/busses/scsi/$MODULE_NAME
if [ -e ~/config/settings/kernel/drivers/$SETTINGS_NAME ]; then
/boot/beos/bin/cp `dirname $0`/$SETTINGS_NAME.sample ~/config/settings/kernel/drivers/
else 
/boot/beos/bin/cp `dirname $0`/$SETTINGS_NAME.sample ~/config/settings/kernel/drivers/$SETTINGS_NAME
fi

mimeset ~/config/add-ons/kernel/busses/scsi/$MODULE_NAME
mimeset ~/config/settings/kernel/drivers/$SETTINGS_NAME

INSTALLED_MESSAGE="The $MODULE_TITLE has been installed on your system.You have to reboot your system before using it."
answer=`alert "$INSTALLED_MESSAGE" "Edit settings" "Reboot" "OK"` 
if [ $answer == "OK" ]; then
  exit
elif [ $answer == "Reboot" ]; then
  shutdown -r
fi

StyledEdit /boot/home/config/settings/kernel/drivers/$SETTINGS_NAME
