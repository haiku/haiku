	#include <Application.h>
	#include <Button.h>
	#include <ColorControl.h>
	#include <Window.h>
	#include <Box.h>
	#include <Alert.h>
	#include <Menu.h>
	#include <MenuBar.h>
	#include <MenuItem.h>
	
	#include "BitmapMenuItem.h"
	
	#ifndef MENU_H 
	#define MENU_H
	
	#include "msg.h"
	
	class FontSizeMenu : public BMenu {
		public:
						FontSizeMenu();
		virtual			~FontSizeMenu();
		virtual void	Update();
		
		menu_info 		info;
		BMenuItem		*fontSizeNine;
		BMenuItem		*fontSizeTen;
		BMenuItem		*fontSizeEleven;
		BMenuItem		*fontSizeTwelve;
		BMenuItem		*fontSizeFourteen;
		BMenuItem		*fontSizeEighteen;
	};
	
	class FontMenu : public BMenu {
		public:
						FontMenu();
		virtual			~FontMenu();
		virtual void	GetFonts();
		virtual void	Update();
		virtual status_t PlaceCheckMarkOnFont(font_family family, font_style style);
		virtual void	ClearAllMarkedItems();
	
		menu_info		info;
		BMenuItem		*fontFamily;
		BMenu			*fontStyleMenu;
		BMenuItem		*fontStyleItem;
	};
	
	class MenuBar : public BMenuBar {
		public:
						MenuBar();
		virtual 		~MenuBar();
				void	set_menu();
				void	build_menu();
		virtual void	Update();
		virtual void 	FrameResized(float width, float height);
		
		BRect 			menu_rect;
		BRect			rect;
		menu_info 		info;
		
		//bitmaps
		BBitmap			*fCtlBmp;
		BBitmap			*fAltBmp;
		BBitmap			*fSep0Bmp;
		BBitmap			*fSep1Bmp;
		BBitmap			*fSep2Bmp;
		
		//seperator submenu
		BMenu			*separatorStyleMenu;
		BMenuItem		*separatorStyleZero;
		BMenuItem		*separatorStyleOne;
		BMenuItem		*separatorStyleTwo;
		
		//others
		FontMenu		*fontMenu;
		FontSizeMenu	*fontSizeMenu;
		BMenuItem		*clickToOpenItem;
		BMenuItem		*alwaysShowTriggersItem;
		BMenuItem		*colorSchemeItem;
		BMenuItem		*separatorStyleItem;
		BMenuItem		*ctlAsShortcutItem;
		BMenuItem		*altAsShortcutItem;
	};
	
	class ColorPicker : public BColorControl {
		public:
						ColorPicker();
		virtual			~ColorPicker();
		virtual	void	MessageReceived(BMessage *msg);
	};
	
	class ColorWindow : public BWindow {
		public:
						ColorWindow();
		virtual			~ColorWindow();
		virtual void	MessageReceived(BMessage *msg);
		
		ColorPicker 	*colorPicker;
		BButton 		*DefaultButton;
		BButton 		*RevertButton;
		menu_info		revert_info;
		menu_info 		info;
	};
		
	class MenuWindow : public BWindow {
		public:
						MenuWindow();
		virtual			~MenuWindow();
		virtual void	MessageReceived(BMessage *msg);
		virtual bool	QuitRequested();
		virtual	void	Update();
				void	Defaults();
		
				bool	revert;
		ColorWindow		*colorWindow;
		BMenuItem 		*toggleItem;
		menu_info		info;
		menu_info		revert_info;
		BRect			rect;	
		BMenu			*menu;
		MenuBar			*menuBar;
		BBox			*menuView;
		BButton 		*revertButton;
		BButton 		*defaultButton;
	};
	
	class MenuApp : public BApplication {
		public:
						MenuApp();
		virtual			~MenuApp();
		virtual void	Update();
		virtual void	MessageReceived(BMessage *msg);
		virtual bool	QuitRequested();
		
		//main
		MenuWindow		*menuWindow;
		BRect			rect;
		menu_info		info;
		BMenu 			*menu;
	};
	
	#endif