#
# Haiku setup for
# XDG Base Directory Specification
#
# http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

export XDG_CONFIG_HOME="`finddir B_USER_SETTINGS_DIRECTORY`"

export XDG_DATA_HOME="`finddir B_USER_NONPACKAGED_DATA_DIRECTORY`"

export XDG_CONFIG_DIRS="`finddir B_SYSTEM_SETTINGS_DIRECTORY`"
# XXX:B_USER_ETC_DIRECTORY?

export XDG_DATA_DIRS="`finddir B_SYSTEM_NONPACKAGED_DATA_DIRECTORY`:\
`finddir B_SYSTEM_DATA_DIRECTORY`"

export XDG_CACHE_HOME="`finddir B_USER_CACHE_DIRECTORY`"

# XXX:TODO
#export XDG_RUNTIME_DIR="`finddir B_USER_VAR_DIRECTORY`/tmp"
