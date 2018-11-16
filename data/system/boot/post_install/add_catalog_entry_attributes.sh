#!/bin/sh

WriteCatalogEntryAttribute()
{
	# $1 : signature
	# $2 : path
	# $3 : context
	
	if ! [ -e "$2" ]
	then
		mkdir --parents "$2"
	fi

	addattr -t string SYS:NAME "$1:$3:$(basename "$2")" "$2"
}


WriteTrackerCatalogEntryAttribute()
{
	WriteCatalogEntryAttribute "x-vnd.Haiku-libtracker" "$1" "$2"
}


WriteDeskbarCatalogEntryAttribute()
{
	WriteCatalogEntryAttribute "x-vnd.Be-TSKB" "$1" "$2"
}


WriteMailCatalogEntryAttribute()
{
	WriteCatalogEntryAttribute "x-vnd.Be-MAIL" "$1" "$2"
}


# TODO: Several of the directories are read-only, so this doesn't work.
WriteTrackerCatalogEntryAttribute \
	"$(finddir B_DESKTOP_DIRECTORY)" B_DESKTOP_DIRECTORY

WriteTrackerCatalogEntryAttribute \
	"$(finddir B_USER_SETTINGS_DIRECTORY)/Tracker/Tracker New Templates" \
	"B_USER_SETTINGS_DIRECTORY/Tracker/Tracker New Templates"

WriteTrackerCatalogEntryAttribute \
	"$(finddir B_TRASH_DIRECTORY)" B_TRASH_DIRECTORY



WriteDeskbarCatalogEntryAttribute \
	"$(finddir B_USER_DESKBAR_DIRECTORY)/Applications" "B_USER_DESKBAR_DIRECTORY/Applications"

WriteDeskbarCatalogEntryAttribute \
	"$(finddir B_USER_DESKBAR_DIRECTORY)/Demos" "B_USER_DESKBAR_DIRECTORY/Demos"

WriteDeskbarCatalogEntryAttribute \
	"$(finddir B_USER_DESKBAR_DIRECTORY)/Desktop applets" "B_USER_DESKBAR_DIRECTORY/Desktop applets"

WriteDeskbarCatalogEntryAttribute \
	"$(finddir B_USER_DESKBAR_DIRECTORY)/Preferences" "B_USER_DESKBAR_DIRECTORY/Preferences"



WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail" "B_USER_DIRECTORY/mail"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/draft" "B_USER_DIRECTORY/mail/draft"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/in" "B_USER_DIRECTORY/mail/in"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/out" "B_USER_DIRECTORY/mail/out"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/queries" "B_USER_DIRECTORY/mail/queries"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/sent" "B_USER_DIRECTORY/mail/sent"

WriteMailCatalogEntryAttribute \
	"$(finddir B_USER_DIRECTORY)/mail/spam" "B_USER_DIRECTORY/mail/spam"

