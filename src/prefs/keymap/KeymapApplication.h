#ifndef OBOS_KEYMAP_APPLICATION_H
#define OBOS_KEYMAP_APPLICATION_H

#include <be/storage/Entry.h>
#include <be/support/List.h>

#define APP_SIGNATURE		"application/x-vnd.OpenBeOS-Keymap"
#define COPY_BUFFER_SIZE	1 * 1024


class KeymapWindow;

class KeymapApplication : public BApplication {
public:
	KeymapApplication();
	void	MessageReceived(BMessage *message);
	BList*	SystemMaps();
	BList*	UserMaps();
	BEntry* CurrentMap();
	bool	UseKeymap( BEntry *keymap );

protected:
	KeymapWindow		*fWindow;

	BList*	EntryList( char *directoryPath );
	bool 	IsValidKeymap( BFile *inFile );
	int		Copy( BFile *original, BFile *copy );
	int		NotifyInputServer();

};

#endif // OBOS_KEYMAP_APPLICATION_H