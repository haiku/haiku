/*
 * ThemesAddon class
 */

#include <Debug.h>
#include <Directory.h>
#include <Message.h>
#include <String.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ThemesAddon.h"


#define DEBUG_TA
#ifdef DEBUG_TA
#define FENTRY PRINT(("ThemesAddon[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif


extern bool CompareMessages(BMessage &a, BMessage &b);


ThemesAddon::ThemesAddon(const char *name, const char *message_name)
	:fImageId(-1),
	fName(NULL),
	fMsgName(NULL),
	fFlags(0L)
{
	fName = strdup(name);
	FENTRY;
	if (message_name)
		fMsgName = strdup(message_name);
	fSettings.MakeEmpty();
}

ThemesAddon::~ThemesAddon()
{
	FENTRY;
	free(fMsgName);
	free(fName);
}

const char *ThemesAddon::Name()
{
	return (const char *)fName;
}

const char *ThemesAddon::Description()
{
	FENTRY;
	return "No description yet.";
}
	
BView		*ThemesAddon::OptionsView()
{
	FENTRY;
	return NULL;
}

status_t	ThemesAddon::RunPreferencesPanel()
{
	FENTRY;
	return B_OK;
}

status_t	ThemesAddon::LoadSettings(BMessage &settings)
{
	FENTRY;
	uint32 flags;
	fSettings = settings;
	if (fSettings.FindInt32("ta:flags", (int32 *)&flags) >= B_OK)
		fFlags = flags;
	return B_OK;
}

status_t	ThemesAddon::SaveSettings(BMessage &settings)
{
	FENTRY;
	status_t err;
	err = fSettings.ReplaceInt32("ta:flags", fFlags);
	settings = fSettings;
	return err;
}

void ThemesAddon::SetAddonFlags(uint32 flags)
{
	fFlags = flags;
}

uint32 ThemesAddon::AddonFlags()
{
	return fFlags;
}

status_t ThemesAddon::AddNames(BMessage &names)
{
	FENTRY;
	BString str;
	if (MessageName())
	str = Name();
	str << " settings";
	names.AddString(MessageName(), str.String());
	return B_OK;
}


status_t ThemesAddon::MyMessage(BMessage &theme, BMessage &mine)
{
	FENTRY;
	BMessage msg;
	status_t err;
	if (!MessageName())
		return B_NAME_NOT_FOUND;
	err = theme.FindMessage(MessageName(), &msg);
	if (err)
		return err;
	mine = msg;
	return B_OK;
}

status_t ThemesAddon::SetMyMessage(BMessage &theme, BMessage &mine)
{
	FENTRY;
	status_t err;
	BMessage msg;
	if (!MessageName())
		return B_NAME_NOT_FOUND;
	err = theme.FindMessage(MessageName(), &msg);
	if (err)
		err = theme.AddMessage(MessageName(), &mine);
	else
		err = theme.ReplaceMessage(MessageName(), &mine);
	return err;
}

const char *ThemesAddon::MessageName()
{
	return (const char *)fMsgName;
}

status_t ThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	FENTRY;
	(void)theme; (void) flags;
	return B_OK;
}

status_t ThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	FENTRY;
	(void)theme; (void) flags;
	return B_OK;
}

status_t ThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	FENTRY;
	(void) flags;
	return B_OK;
}


status_t ThemesAddon::BackupCurrent(BMessage &theme)
{
	FENTRY;
	return MakeTheme(theme);
}

status_t ThemesAddon::RestoreCurrent(BMessage &theme)
{
	FENTRY;
	return ApplyTheme(theme);
}

/* by default, try to find the addon's specific message
 * and compare them. Some addons don't add any message,
 * they'll have to do comparison by hand.
 */
status_t ThemesAddon::CompareToCurrent(BMessage &theme)
{
	FENTRY;
	BMessage current, a, b;
	BackupCurrent(current);
	status_t err;
	err = theme.FindMessage(MessageName(), &a);
	if (err)
		return err;
	err = current.FindMessage(MessageName(), &b);
	if (err)
		return err;
	if (!CompareMessages(a, b))
		return 1;
	
	return B_OK;
}

	
status_t ThemesAddon::InstallFiles(BMessage &theme, BDirectory &folder)
{
	FENTRY;
	(void)theme; (void)folder;
	return B_OK;
}

status_t ThemesAddon::BackupFiles(BMessage &theme, BDirectory &folder)
{
	FENTRY;
	(void)theme; (void)folder;
	return B_OK;
}


	/* private */

void ThemesAddon::SetImageId(image_id id)
{
	fImageId = id;
}

image_id ThemesAddon::ImageId()
{
	return fImageId;
}

