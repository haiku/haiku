/*
 * Copyright 2007-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _THEME_IMPORTER_H
#define _THEME_IMPORTER_H
/*
 * ThemeImporter class header
 */

class BDirectory;
class BMessage;
class BString;

namespace Z {
namespace ThemeManager {

class ThemeManager;

class ThemeImporter {
public:
			ThemeImporter(const char *name);
virtual		~ThemeImporter();
	
	/* presentation */
	
virtual	const char	*Name(); /* pretty name */
virtual const char	*Description(); /* tooltip... */
					// if you override, call it first
virtual status_t	LoadSettings(BMessage &settings);
					// if you override, call it last
virtual status_t	SaveSettings(BMessage &settings);
void		SetFlags(uint32 flags);
uint32		Flags();
	
	/* Theme manipulation */
	
virtual status_t	FetchThemes();
virtual status_t	ImportNextTheme(BMessage **theme);
virtual status_t	EndImports();
	
	/*  */
	
protected:

friend class Z::ThemeManager::ThemeManager;
	BString	fName;
	uint32	fFlags;
	BMessage fSettings;
};

} // ns ThemeManager
} // ns Z

using namespace Z::ThemeManager;

#endif /* _THEME_IMPORTER_H */
