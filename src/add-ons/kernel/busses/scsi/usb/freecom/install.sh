#!/bin/sh
##########################################################
##
##     USB Storage Module installation script
##
##  This file is a part of BeOS USB SCSI interface module project.
##  Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
##  All rights reserved.
##
##  $Source: /cvsroot/sis4be/usb_scsi/freecom/install.sh,v $
##  $Author: zharik $
##  $Revision: 1.1 $
##  $Date: 2005/04/09 22:16:17 $
##
##########################################################

MODULE_TITLE="FREECOM extension for USB Storage Module"
MODULE_NAME=freecom
SETTINGS_NAME=usb_scsi

answer=`alert "Do you really want to install the $MODULE_TITLE ?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/beos/bin/install -d ~/config/add-ons/kernel/generic/usb_scsi_extension/
/boot/beos/bin/install `dirname $0`/$MODULE_NAME ~/config/add-ons/kernel/generic/usb_scsi_extension/$MODULE_NAME

#  TODO!!!
#if [ -e ~/config/settings/kernel/drivers/$SETTINGS_NAME ]; then
#/boot/beos/bin/cp `dirname $0`/$SETTINGS_NAME.sample ~/config/settings/kernel/drivers/
#else 
#/boot/beos/bin/cp `dirname $0`/$SETTINGS_NAME.sample ~/config/settings/kernel/drivers/$SETTINGS_NAME
#fi

mimeset ~/config/add-ons/kernel/generic/usb_scsi_extension/$MODULE_NAME

INSTALLED_MESSAGE="The $MODULE_TITLE has been installed on your system. You have to review/edit settings for your FREECOM device."
alert "$INSTALLED_MESSAGE" "Edit settings"

StyledEdit /boot/home/config/settings/kernel/drivers/$SETTINGS_NAME
