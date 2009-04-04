#!/bin/sh

# Make sure all apps have a MIME DB entry.

SYSTEM=$(/bin/finddir B_SYSTEM_DIRECTORY 2>/dev/null || echo "/boot/beos")

mimeset -apps -f "$SYSTEM/apps"
mimeset       -f "$SYSTEM/documentation"
mimeset -apps -f "$SYSTEM/preferences"
mimeset -apps -f "$SYSTEM/servers"
mimeset -apps -f "/boot/apps"
