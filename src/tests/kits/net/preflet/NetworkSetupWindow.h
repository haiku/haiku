#ifndef NETWORKSETUPWINDOW_H
#define NETWORKSETUPWINDOW_H


#include <InterfaceKit.h>

#define PROFILE_LABEL 			"Profile:"
#define NEW_PROFILE_LABEL		"New" B_UTF8_ELLIPSIS
#define COPY_PROFILE_LABEL		"Copy" B_UTF8_ELLIPSIS
#define DELETE_PROFILE_LABEL 	"Delete"

#define MANAGE_PROFILES_LABEL		"Manage profiles" B_UTF8_ELLIPSIS

#define SHOW_LABEL			"Show:"

#define DONT_TOUCH_LABEL	"Prevent unwanted changes"
#define SAVE_LABEL			"Save"
#define HELP_LABEL			"Help"
#define REVERT_LABEL		"Revert"
#define APPLY_NOW_LABEL		"Apply Now"

class NetworkSetupWindow : public BWindow
{
	public:
		// Constructors, destructors, operators...
		NetworkSetupWindow(const char *title);
		~NetworkSetupWindow();
							
		typedef BWindow	 		inherited;

		// Virtual function overrides
	public:	
		// public constantes
		enum
			{
			SELECT_PROFILE_MSG			= 'prof',
			MANAGE_PROFILES_MSG			= 'mngp',
			NEW_PROFILE_MSG				= 'newp',
			COPY_PROFILE_MSG			= 'cpyp',
			DELETE_PROFILE_MSG			= 'delp',
			SHOW_MSG					= 'show',
			HELP_MSG					= 'help',
			DONT_TOUCH_MSG				= 'lock',
			APPLY_NOW_MSG 				= 'aply',
			REVERT_MSG					= 'rvrt'
			};
				
		bool			QuitRequested();
		void			MessageReceived(BMessage * msg);
		
		// From here, it's none of your business! ;-)
	private:
		void BuildProfilesMenu(BMenu *menu, int32 msg);
		void BuildShowMenu(BMenu *menu, int32 msg);
	
		BButton *	fHelpButton;
		BButton *	fRevertButton;
		BButton *	fApplyNowButton;

		BBox *		fPanel;
		BView *		fAddonView;
		BRect		fMinAddonViewRect;
};

#endif // ifdef NETWORKSETUPWINDOW_H

