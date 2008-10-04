#!/bin/sh

# Make sure all apps have a MIME DB entry.

mimeset -apps -f /boot/beos/apps
mimeset -f /boot/beos/documentation
mimeset -apps -f /boot/beos/preferences
mimeset -apps -f /boot/beos/system/servers
mimeset -apps -f /boot/apps
