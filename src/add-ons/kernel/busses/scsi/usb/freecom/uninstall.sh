#!/bin/sh
##########################################################
##
##     USB Storage Module uninstallation script
##
##  This file is a part of BeOS USB SCSI interface module project.
##  Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
##  All rights reserved.
##
##  $Source: /cvsroot/sis4be/usb_scsi/freecom/uninstall.sh,v $
##  $Author: zharik $
##  $Revision: 1.1 $
##  $Date: 2005/04/09 22:16:17 $
##
##########################################################

DRIVER_TITLE="FREECOM extension for USB Storage Module"
SETTINGS_NAME=usb_scsi
DRIVER_TARGET_NAME=freecom

answer=`alert "Do you really want to uninstall the $DRIVER_TITLE ?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

rm ~/config/add-ons/kernel/generic/usb_scsi_extension/$DRIVER_TARGET_NAME

answer=`alert "The $DRIVER_TITLE has been removed from your system. You can now remove FREECOM device description for settings file." "OK" "Edit settings"`
if [ $answer == "OK" ]; then
  exit
fi

StyledEdit /boot/home/config/settings/kernel/drivers/$SETTINGS_NAME
