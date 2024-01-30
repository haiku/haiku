#!/bin/sh

# install ProcessController, NetworkStatus, PowerStatus & volume control in the Deskbar
/boot/system/apps/ProcessController -deskbar
/boot/system/apps/NetworkStatus --deskbar
/boot/system/apps/PowerStatus --deskbar
/boot/system/bin/desklink --add-volume

# install KeymapSwitcher for certain locales
if [[ `locale -l` =~ ^(ru|uk|be)$ ]]; then
   /boot/system/preferences/KeymapSwitcher --deskbar
fi
