#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <Locker.h>
#include <List.h>
#include <Menu.h>
#include <InterfaceDefs.h>

class DisplayDriver;
class RootLayer;
class Screen;
class Layer;
class BMessage;
class PortMessage;
class WinBorder;

class Desktop
{
public:
	// startup methods
	Desktop(void);
	~Desktop(void);
	void Init(void);

	// 1-BigScreen or n-SmallScreens
	void InitMode(void);
	
	// Methods for multiple monitors.
	Screen *ScreenAt(int32 index) const;
	int32 ScreenCount(void) const;
	Screen *ActiveScreen(void) const { return fActiveScreen; }
	void SetActiveScreen(int32 index);
	
	void SetActiveRootLayerByIndex(int32 listIndex);
	void SetActiveRootLayer(RootLayer* rl);
	RootLayer* RootLayerAt(int32 index);
	RootLayer* ActiveRootLayer(void) const;
	int32 ActiveRootLayerIndex(void) const;
	int32 CountRootLayers(void) const;

	// Stuff for easy access
	DisplayDriver *GetGfxDriver(void);
	void SetWorkspace(int32 index);
	
	// Methods for layer(WinBorder) manipulation.
	void AddWinBorder(WinBorder* winBorder);
	void RemoveWinBorder(WinBorder* winBorder);
	bool HasWinBorder(WinBorder* winBorder);
	void SetFrontWinBorder(WinBorder* winBorder);
	void SetFocusWinBorder(WinBorder* winBorder);
	WinBorder* FrontWinBorder(void) const;
	WinBorder* FocusWinBorder(void) const;
	
	// Input related methods
	void MouseEventHandler(PortMessage *msg);
	void KeyboardEventHandler(PortMessage *msg);
	
	void SetDragMessage(BMessage* msg);
	BMessage* DragMessage(void) const;
	
	// Methods for various desktop stuff handled by the server
	void SetScrollBarInfo(const scroll_bar_info &info);
	scroll_bar_info	ScrollBarInfo(void) const;
	
	void SetMenuInfo(const menu_info &info);
	menu_info MenuInfo(void) const;
	
	
	void UseFFMouse(const bool &useffm);
	bool FFMouseInUse(void) const;
	void SetFFMouseMode(const mode_mouse &value);
	mode_mouse FFMouseMode(void) const;
	
	bool ReadWorkspaceData(void);
	void SaveWorkspaceData(void);
	
	// Debugging methods
	void PrintToStream(void);
	void PrintVisibleInRootLayerNo(int32 no);
	
	// "Private" to app_server :-) - means they should not be used very much
	void RemoveSubsetWindow(WinBorder* wb);

private:
	BLocker fLayerLock;
	BList fWinBorderList;
	
	BMessage *fDragMessage;
	
	BList fRootLayerList;
	RootLayer* fActiveRootLayer;
	WinBorder* fFrontWinBorder;
	WinBorder* fFocusWinBorder;
	
	BList fScreenList;
	
	scroll_bar_info	fScrollBarInfo;
	menu_info fMenuInfo;
	mode_mouse fMouseMode;
	bool fFFMouseMode;
	Screen *fActiveScreen;
};

extern Desktop* desktop;

#endif // _DESKTOP_H_
