#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <Locker.h>
#include <List.h>
#include <Menu.h>
#include <InterfaceDefs.h>

class RootLayer;
class Screen;
class Layer;
class BMessage;
class PortMessage;
class WinBorder;
class DisplayDriver;

class Desktop
{
public:
				// startup methods
								Desktop(void);
								~Desktop(void);
			void				Init(void);
			void				InitMode(void);			// 1-BigScreen or n-SmallScreens

				// Methods for multiple monitors.
			Screen*				ScreenAt(int32 index) const;
			int32				ScreenCount() const;
			Screen*				CursorScreen() const;

			void				SetActiveRootLayerByIndex(int32 listIndex);
			void				SetActiveRootLayer(RootLayer* rl);
			RootLayer*			RootLayerAt(int32 index);
			RootLayer*			ActiveRootLayer() const;
			int32				ActiveRootLayerIndex() const;
			int32				CountRootLayers() const;
			DisplayDriver*		GetDisplayDriver() const;

				// Methods for layer(WinBorder) manipulation.
			void				AddWinBorder(WinBorder* winBorder);
			void				RemoveWinBorder(WinBorder* winBorder);
			bool				HasWinBorder(WinBorder* winBorder);
			void				SetFrontWinBorder(WinBorder* winBorder);
			void				SetFoocusWinBorder(WinBorder* winBorder);
			WinBorder*			FrontWinBorder(void) const;
			WinBorder*			FocusWinBorder(void) const;

				// Input related methods
			void				MouseEventHandler(PortMessage *msg);
			void				KeyboardEventHandler(PortMessage *msg);

			void				SetDragMessage(BMessage* msg);
			BMessage*			DragMessage(void) const;

				// Methods for various desktop stuff handled by the server
			void				SetScrollBarInfo(const scroll_bar_info &info);
			scroll_bar_info		ScrollBarInfo(void) const;

			void				SetMenuInfo(const menu_info &info);
			menu_info			MenuInfo(void) const;


			void				UseFFMouse(const bool &useffm);
			bool				FFMouseInUse(void) const;
			void				SetFFMouseMode(const mode_mouse &value);
			mode_mouse			FFMouseMode(void) const;

			bool				ReadWorkspaceData(void);
			void				SaveWorkspaceData(void);
			
// Debugging methods
			void				PrintToStream();
			void				PrintVisibleInRootLayerNo(int32 no);

// "Private" to app_server :-) - means they should not be used very much
			void				RemoveSubsetWindow(WinBorder* wb);
			WinBorder*			FindWinBorderByServerWindowToken(int32 token);

			BLocker				fGeneralLock;
			BLocker				fLayerLock;
			BList				fWinBorderList;

private:

			BMessage			*fDragMessage;

			BList				fRootLayerList;
			RootLayer*			fActiveRootLayer;
			WinBorder*			fFrontWinBorder;
			WinBorder*			fFocusWinBorder;

			BList				fScreenList;
			int32				fScreenMode;	// '1' or 'n' Screen(s). Not sure it's needed though.
			Screen*				fCursorScreen;

			scroll_bar_info		fScrollBarInfo;
			menu_info			fMenuInfo;
			mode_mouse			fMouseMode;
			bool				fFFMouseMode;
};

extern Desktop* desktop;

#endif _DESKTOP_H_
