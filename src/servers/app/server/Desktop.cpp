#include "Desktop.h"

#include <Message.h>
#include "RootLayer.h"
#include "ServerScreen.h"
#include "Layer.h"
#include "PortMessage.h"
#include "DisplayDriver.h"
#include "AccelerantDriver.h"
#include "ViewDriver.h"
#include "WinBorder.h"
#include "Workspace.h"
#include "Globals.h"
#include "ServerWindow.h"
#include "ServerConfig.h"

#include <Region.h>
#include <stdio.h>

Desktop *desktop=NULL;

Desktop::Desktop(void)
{
	fDragMessage		= NULL;
	fActiveRootLayer	= NULL;
	fFrontWinBorder		= NULL;
	fFocusWinBorder		= NULL;
	fActiveScreen		= NULL;
	Init();
}

Desktop::~Desktop(void)
{
	if (fDragMessage)
		delete fDragMessage;
	
	void *ptr;
	for(int32 i=0; (ptr=fRootLayerList.ItemAt(i)); i++)
		delete (RootLayer*)ptr;

	for(int32 i=0; (ptr=fScreenList.ItemAt(i)); i++)
		delete (Screen*)ptr;

	for(int32 i=0; (ptr=fWinBorderList.ItemAt(i)); i++)
		delete (WinBorder*)ptr;
}

void Desktop::Init(void)
{
	DisplayDriver	*driver = NULL;
	int32 driverCount = 0;
	bool initDrivers = true;

	while(initDrivers)
	{
		#ifdef TEST_MODE
			driver 	= new ViewDriver();
		#else
		
			// If we're using the AccelerantDriver for rendering, eventually we will loop through
			// drivers until one can't initialize in order to support multiple monitors. For now,
			// we'll just load one and be done with it.
			driver = new AccelerantDriver();
		#endif
		
		if(driver->Initialize())
		{
			driverCount++;
			
			Screen *sc = new Screen(driver, BPoint(640, 480), B_RGB32, driverCount);
			
			// TODO: be careful, it may fail to initialize! - Monitor may not support 640x480
			fScreenList.AddItem(sc);
			
			// TODO: remove this when you have a real Driver.
			if (driverCount == 2)
				initDrivers	= false;
		}
		else
		{
			driver->Shutdown();
			delete	driver;
			driver	= NULL;
			initDrivers	= false;
		}
	}

	if (driverCount < 1)
		delete this;

	InitMode();
	
	SetActiveRootLayerByIndex(0);
}

void Desktop::InitMode(void)
{
	// this is init mode for n-SS.
	for (int32 i=0; i<fScreenList.CountItems(); i++)
	{
		char		name[32];
		sprintf(name, "RootLayer %ld", i+1);
		
		Screen *screens[1];		
		screens[0]= (Screen*)fScreenList.ItemAt(i);
		
		RootLayer *rl = new RootLayer(name, 4, this);
		rl->SetScreens(screens, 1, 1);
		
		fRootLayerList.AddItem(rl);
	}
}

// Methods for multiple monitors.
Screen* Desktop::ScreenAt(int32 index) const
{
	Screen	*sc;
	sc = static_cast<Screen*>(fScreenList.ItemAt(index));

	return sc;
}

int32 Desktop::ScreenCount(void) const
{
	return fScreenList.CountItems();
}

void Desktop::SetActiveScreen(int32 index)
{
	if(index<0 || index>fScreenList.CountItems()-1)
		return;
	
	// TODO: Once we get multiple monitor support going officially, we'll likely
	// need to do something with the cursor here.
	
	fActiveScreen=(Screen*)fScreenList.ItemAt(index);
}

void Desktop::SetActiveRootLayerByIndex(int32 listIndex)
{
	RootLayer	*rl;
	rl = RootLayerAt(listIndex);
	if (rl)
		SetActiveRootLayer(rl);
}

void Desktop::SetActiveRootLayer(RootLayer* rl)
{
	if (fActiveRootLayer == rl)
		return;

	fActiveRootLayer	= rl;
	
	// also set he new front and focus
	SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());
	SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());
	
	// TODO: other tasks required when this happens. I don't know them now.
	// Rebuild & Invalidate
	// hide the mouse in the old ActiveRootLayer
	// show the mouse in new ActiveRootLayer
}

RootLayer* Desktop::ActiveRootLayer(void) const
{
	return fActiveRootLayer;
}

int32 Desktop::ActiveRootLayerIndex(void) const
{
	int32		rootLayerCount = CountRootLayers(); 
	for(int32 i=0; i<rootLayerCount; i++){
		if(fActiveRootLayer == (RootLayer*)(fRootLayerList.ItemAt(i)))
			return i;
	}
	return -1;
}

RootLayer* Desktop::RootLayerAt(int32 index)
{
	RootLayer	*rl;
	rl = static_cast<RootLayer*>(fRootLayerList.ItemAt(index));

	return rl;
}

int32 Desktop::CountRootLayers(void) const{
	return fRootLayerList.CountItems();
}

// Methods for layer(WinBorder) manipulation.

void Desktop::AddWinBorder(WinBorder* winBorder)
{
	if(fWinBorderList.HasItem(winBorder))
		return;

	// special case for Tracker background window.
	if (winBorder->GetServerFeel() == B_SYSTEM_LAST)
	{
		// it's added in all RottLayers
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
		{
			((RootLayer*)fRootLayerList.ItemAt(i))->AddWinBorder(winBorder);
		}
	}
	else
	{
		// other windows are added to the current RootLayer only.
		ActiveRootLayer()->AddWinBorder(winBorder);
	}
	
	// add that pointer to user winboder list so that we can keep track of them.		
	fWinBorderList.AddItem(winBorder);

	SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());
	SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());
}

void Desktop::RemoveWinBorder(WinBorder* winBorder)
{
	if(winBorder->GetServerFeel() == B_SYSTEM_LAST)
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->RemoveWinBorder(winBorder);
	else
		winBorder->GetRootLayer()->RemoveWinBorder(winBorder);

	if (winBorder == fFrontWinBorder)
		SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());

	if (winBorder == fFocusWinBorder)
		SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());

	fWinBorderList.RemoveItem(winBorder);
}

bool Desktop::HasWinBorder(WinBorder* winBorder)
{
	return fWinBorderList.HasItem(winBorder);
}

void Desktop::SetFrontWinBorder(WinBorder* winBorder)
{
	// TODO: implement
	fFrontWinBorder		= winBorder;
}

void Desktop::SetFocusWinBorder(WinBorder* winBorder)
{
	// TODO: implement
	fFocusWinBorder		= winBorder;
}

WinBorder* Desktop::FrontWinBorder(void) const
{
	return fActiveRootLayer->ActiveWorkspace()->FrontLayer();
}

WinBorder* Desktop::FocusWinBorder(void) const
{
	return fActiveRootLayer->ActiveWorkspace()->FocusLayer();
}

// Input related methods

void Desktop::MouseEventHandler(PortMessage *msg)
{
}

void Desktop::KeyboardEventHandler(PortMessage *msg)
{
}

void Desktop::SetDragMessage(BMessage* msg)
{
	if (fDragMessage)
	{
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage	= new BMessage(*msg);
}

BMessage* Desktop::DragMessage(void) const
{
	return fDragMessage;
}

				// Methods for various desktop stuff handled by the server
void Desktop::SetScrollBarInfo(const scroll_bar_info &info)
{
	fScrollBarInfo	= info;
}

scroll_bar_info Desktop::ScrollBarInfo(void) const
{
	return fScrollBarInfo;
}

void Desktop::SetMenuInfo(const menu_info &info)
{
	fMenuInfo	= info;
}

menu_info Desktop::MenuInfo(void) const
{
	return fMenuInfo;
}

void Desktop::UseFFMouse(const bool &useffm)
{
	fFFMouseMode	= useffm;
}

bool Desktop::FFMouseInUse(void) const
{
	return fFFMouseMode;
}

void Desktop::SetFFMouseMode(const mode_mouse &value)
{
	fMouseMode	= value;
}

mode_mouse Desktop::FFMouseMode(void) const
{
	return fMouseMode;
}

bool Desktop::ReadWorkspaceData(void)
{
	// TODO: implement
	return true;
}

void Desktop::SaveWorkspaceData(void)
{
	// TODO: implement
}

void Desktop::RemoveSubsetWindow(WinBorder* wb)
{
	WinBorder		*winBorder = NULL;
	int32			count = fWinBorderList.CountItems();

	for(int32 i=0; i < count; i++)
	{
		winBorder	= static_cast<WinBorder*>(fWinBorderList.ItemAt(i));
		if (winBorder->GetServerFeel() == B_NORMAL_FEEL)
			winBorder->Window()->GetFMWList()->RemoveItem(wb);
	}
	
	RootLayer		*rl = winBorder->GetRootLayer();
	int32			countWKs = rl->WorkspaceCount();
	for (int32 i=0; i < countWKs; i++)
	{
		rl->WorkspaceAt(i+1)->RemoveLayerPtr(wb);
	}
}

void Desktop::PrintToStream(void)
{
	printf("RootLayer List:\n=======\n");
	for(int32 i=0; i<fRootLayerList.CountItems(); i++){
		printf("\t%s\n", ((RootLayer*)fRootLayerList.ItemAt(i))->GetName());
		((RootLayer*)fRootLayerList.ItemAt(i))->PrintToStream();
		printf("-------\n");
	}
	printf("=======\nActive RootLayer: %s\n", fActiveRootLayer? fActiveRootLayer->GetName(): "NULL");
//	printf("Active WinBorder: %s\n", fActiveWinBorder? fActiveWinBorder->Name(void): "NULL");
	
	printf("Screen List:\n");
	for(int32 i=0; i<fScreenList.CountItems(); i++){
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
	}
	
}

void Desktop::PrintVisibleInRootLayerNo(int32 no)
{
	if (no<0 || no>=fRootLayerList.CountItems())
		return;

	printf("Visible windows in RootLayer %ld, Workspace %ld\n",
		ActiveRootLayerIndex(), ActiveRootLayer()->ActiveWorkspaceIndex());
	WinBorder	*wb = NULL;
	Workspace	*ws = ActiveRootLayer()->ActiveWorkspace();
	for(wb = (WinBorder*)ws->GoToTopItem(); wb != NULL; wb = (WinBorder*)ws->GoToLowerItem()){
		if (!wb->IsHidden())
			wb->PrintToStream();
	}
}

DisplayDriver *Desktop::GetGfxDriver(void)
{
	return fActiveScreen->GetDriver();
}

void Desktop::SetWorkspace(int32 index)
{
	fActiveRootLayer->SetActiveWorkspaceByIndex(index);
}
