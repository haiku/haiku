#ifndef OBOS_KEYMAP_WINDOW_H
#define OBOS_KEYMAP_WINDOW_H


#include <interface/Window.h>
#include <support/List.h>
#include <interface/MenuBar.h>


#if !DEBUG
	#define WINDOW_TITLE				"Keymap"
#else
	#define WINDOW_TITLE				"OBOS Keymap"
#endif //DEBUG
#define WINDOW_LEFT_TOP_POSITION	BPoint( 80, 25 )
#define WINDOW_DIMENSIONS			BRect( 0,0, 612,256 )

class KeymapListItem;
class KeymapApplication;

class MapView : public BView
{
public:
	MapView(BRect rect, const char *name);
	void Draw(BRect rect);
	void DrawKey(BRect rect, bool pressed, bool vertical = false);
	void DrawBorder(BRect borderRect);
	void Pulse();

	key_info fOldKeyInfo;
};


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
	
	MapView				*fMapView;

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
