#!/bin/sh

# install ProcessController, NetworkStatus & volume control in the Deskbar
/boot/system/apps/ProcessController -deskbar
/boot/system/apps/NetworkStatus --deskbar
/boot/system/bin/desklink --add-volume
