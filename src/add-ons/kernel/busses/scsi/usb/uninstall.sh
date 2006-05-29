#!/bin/sh
##########################################################
##
##     USB Storage Module uninstallation script
##
##  This file is a part of BeOS USB SCSI interface module project.
##  Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
##  All rights reserved.
##
##  $Source: /cvsroot/sis4be/usb_scsi/uninstall.sh,v $
##  $Author: zharik $
##  $Revision: 1.3 $
##  $Date: 2004/06/06 09:19:47 $
##
##########################################################

DRIVER_TITLE="USB Storage Module"
DRIVER_NAME=usb_scsi
DRIVER_TARGET_NAME=usb

answer=`alert "Do you really want to uninstall the $DRIVER_TITLE ?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

rm ~/config/add-ons/kernel/busses/scsi/$DRIVER_TARGET_NAME
#rm ~/config/settings/kernel/drivers/$DRIVER_NAME

answer=`alert "The $DRIVER_TITLE has been removed from your system.You need to reboot your system to finish uninstalletion" "Reboot" "OK"`
if [ $answer == "Reboot" ]; then
  shutdown -r
fi
