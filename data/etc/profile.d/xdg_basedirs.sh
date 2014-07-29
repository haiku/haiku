#
# Haiku setup for
# XDG Base Directory Specification
#
# http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

# defaults to ~/.config
export XDG_CONFIG_HOME="`finddir B_USER_SETTINGS_DIRECTORY`"

# defaults to ~/.local/share
export XDG_DATA_HOME="`finddir B_USER_NONPACKAGED_DATA_DIRECTORY`"

# defaults to /etc/xdg
export XDG_CONFIG_DIRS="`finddir B_SYSTEM_SETTINGS_DIRECTORY`"
# XXX: Should we add B_USER_ETC_DIRECTORY?

# default to /usr/local/share/:/usr/share/
export XDG_DATA_DIRS="`finddir B_SYSTEM_NONPACKAGED_DATA_DIRECTORY`:\
`finddir B_SYSTEM_DATA_DIRECTORY`"

# defaults to ~/.cache
export XDG_CACHE_HOME="`finddir B_USER_CACHE_DIRECTORY`"

# TODO:
# This one is used for session stuff (sockets, pipes...)
# It must be owned by the user, with permissions 0700.
# It is supposed to be cleaned up on last log-out.
# Apps will fall back to /tmp usually anyway.
#export XDG_RUNTIME_DIR="`finddir B_USER_VAR_DIRECTORY`/tmp"
