#ifndef OBOS_KEYMAP_WINDOW_H
#define OBOS_KEYMAP_WINDOW_H


#include <be/interface/Window.h>
#include <be/support/List.h>
#include <be/interface/MenuBar.h>


#if !DEBUG
	#define WINDOW_TITLE				"Keymap"
#else
	#define WINDOW_TITLE				"OBOS Keymap"
#endif //DEBUG
#define WINDOW_LEFT_TOP_POSITION	BPoint( 80, 25 )
#define WINDOW_DIMENSIONS			BRect( 0,0, 612,256 )

class KeymapListItem;
class KeymapApplication;

class KeymapWindow : public BWindow {
public:
			KeymapWindow( BRect frame );
	bool	QuitRequested();
	void	MessageReceived( BMessage* message );

protected:
	KeymapApplication	*fApplication;
	BView				*fPlaceholderView;
	BListView			*fSystemListView;
	BListView			*fUserListView;
	// the map that's currently highlighted
	BEntry				*fSelectedMap;
	BButton				*fUseButton;
	BButton				*fRevertButton;
	const char			*title;

	BMenuBar		*AddMenuBar();
	void			AddMaps();
	BList			*ListItemsFromEntryList( BList * entryList);
	KeymapListItem* ItemFromEntry( BEntry *entry );
	void			HandleSystemMapSelected( BMessage* );
	void			HandleUserMapSelected( BMessage* );
	void			HandleMapSelected( BMessage *selectionMessage,
						BListView * selectedView, BListView * otherView );
	void			UseKeymap();

};

#endif // OBOS_KEYMAP_WINDOW_H
