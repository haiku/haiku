#
# colorls.sh - set up the ls color preferences.
#
# The environment variable LS_COLORS maintains colon (:) seperated list of
# color preferences to be used by ls program. This script is placed in 
# /etc/profile.d/ directory that would make sure the script is executed on
# before a terminal is started. The script sets LS_COLORS environment with
# suitable color setting for Haiku terminal.
#
# Copyright 2012 Haiku, Inc. All rights reserved. 
# Distributed under the terms of the MIT License.
#
# Author:
#	Prasad Joshi <prasadjoshi.linux@gmail.com>
#

eval `dircolors -b`
