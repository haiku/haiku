#ifndef _MS_THEME_IMPORTER_H
#define _MS_THEME_IMPORTER_H
/*
 * MSTheme Importer class header
 */

#include <List.h>

#include "ThemeImporter.h"

namespace Z {
namespace ThemeManager {

class MSThemeImporter : public ThemeImporter {
public:
			MSThemeImporter();
virtual		~MSThemeImporter();
	
	/* presentation */
	
virtual const char	*Description(); /* tooltip... */
	
	/* Theme manipulation */
	
virtual status_t	FetchThemes();
virtual status_t	ImportNextTheme(BMessage **theme);
virtual status_t	EndImports();
	
	/*  */
	
private:

bool				ScanDirectory(BDirectory &dir, int depth=-1);
status_t			ParseWinPath(BDirectory &rootDir, const char *from, BPath &to);

friend class Z::ThemeManager::ThemeManager;
	BList			fEntryList;
};

} // ns ThemeManager
} // ns Z

using namespace Z::ThemeManager;

#endif /* _MS_THEME_IMPORTER_H */
