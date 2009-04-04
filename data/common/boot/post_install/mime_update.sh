#!/bin/sh

# Make sure all apps have a MIME DB entry.

mimeset -apps -f /boot/system/apps
mimeset -f /boot/system/documentation
mimeset -apps -f /boot/system/preferences
mimeset -apps -f /boot/system/servers
mimeset -apps -f /boot/apps
