#ifndef NETWORKSETUPWINDOW_H
#define NETWORKSETUPWINDOW_H


#include <InterfaceKit.h>

class NetworkSetupWindow : public BWindow
{
	public:
		NetworkSetupWindow(const char *title);
		~NetworkSetupWindow();
							
		typedef BWindow	 		inherited;

		enum {
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
		void			MessageReceived(BMessage* msg);
		void			AttachedToWindow();

	private:
		void _BuildProfilesMenu(BMenu* menu, int32 msg);
		void _BuildShowMenu(BMenu* menu, int32 msg);
	
		BButton*	fHelpButton;
		BButton*	fRevertButton;
		BButton*	fApplyNowButton;

		BBox*		fPanel;
		BView*		fAddonView;
		BRect		fMinAddonViewRect;
};

#endif // ifdef NETWORKSETUPWINDOW_H
