#ifndef NETWORKWINDOW_H
#define NETWORKWINDOW_H


#include <InterfaceKit.h>

#define PROFILE_LABEL 		"Settings profile:"
#define NEW_PROFILE_LABEL		"New" B_UTF8_ELLIPSIS
#define RENAME_PROFILE_LABEL	"Rename" B_UTF8_ELLIPSIS
#define DELETE_PROFILE_LABEL 	"Delete"

#define SHOW_LABEL			" Show:"

#define SAVE_LABEL			"Save"
#define APPLY_NOW_LABEL		"Apply Now"

class NetworkWindow : public BWindow
{
	public:
		// Constructors, destructors, operators...
		NetworkWindow(const char *title);
		~NetworkWindow();
							
		typedef BWindow	 		inherited;

		// Virtual function overrides
	public:	
		// public constantes
		enum
			{
			SELECT_PROFILE_MSG			= 'prof',
			NEW_PROFILE_MSG				= 'newp',
			RENAME_PROFILE_MSG			= 'renp',
			DELETE_PROFILE_MSG			= 'delp',
			SHOW_MSG					= 'dest',
			APPLY_NOW_MSG 				= 'strt'
			};
				
		virtual bool			QuitRequested();
		virtual void			MessageReceived(BMessage * msg);
		
		// From here, it's none of your business! ;-)
	private:
		void BuildProfilesMenu(BMenu *menu, int32 msg);
		void BuildShowMenu(BMenu *menu, int32 msg);
	
		BBox 		*fPanel;
		BMenuField 	*fProfilesMenu;
		BMenuField 	*fShowMenu;
		BButton		*fRenameButton;
		BButton		*fDeleteButton;
		BButton 	*fApplyButton;
		BRect 		fShowRect;
		BView 		*fShowView;
};

#endif // ifdef NETWORKWINDOW_H

