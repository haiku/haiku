/*
 * ThemeImporter class
 */

#include <Debug.h>
#include <Directory.h>
#include <Message.h>
#include <String.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ThemeImporter.h"


#define DEBUG_TI
#ifdef DEBUG_TI
#define FENTRY PRINT(("ThemesImporter[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif


ThemeImporter::ThemeImporter(const char *name)
	:fName(name),
	fFlags(0L)
{
	FENTRY;
	fSettings.MakeEmpty();
}


ThemeImporter::~ThemeImporter()
{
	FENTRY;
}


const char *
ThemeImporter::Name()
{
	return fName.String();
}


const char *
ThemeImporter::Description()
{
	FENTRY;
	return "No description yet.";
}
	

status_t
ThemeImporter::LoadSettings(BMessage &settings)
{
	FENTRY;
	uint32 flags;
	fSettings = settings;
	if (fSettings.FindInt32("ta:flags", (int32 *)&flags) >= B_OK)
		fFlags = flags;
	return B_OK;
}


status_t
ThemeImporter::SaveSettings(BMessage &settings)
{
	FENTRY;
	status_t err;
	err = fSettings.ReplaceInt32("ta:flags", fFlags);
	settings = fSettings;
	return err;
}


void
ThemeImporter::SetFlags(uint32 flags)
{
	fFlags = flags;
}


uint32
ThemeImporter::Flags()
{
	return fFlags;
}


status_t
ThemeImporter::FetchThemes()
{
	FENTRY;
	return B_OK;
}


status_t
ThemeImporter::ImportNextTheme(BMessage **theme)
{
	FENTRY;
	return ENOENT;
}


status_t
ThemeImporter::EndImports()
{
	FENTRY;
	return B_OK;
}

