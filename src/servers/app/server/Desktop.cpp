#include <stdio.h>
#include <Region.h>
#include <Message.h>

#include "Desktop.h"
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

//#define REAL_MODE

Desktop::Desktop(void){
//	desktop				= this;

	fDragMessage		= NULL;
	fActiveRootLayer	= NULL;
	fFrontWinBorder		= NULL;
	fFocusWinBorder		= NULL;
	fActiveScreen		= NULL;
}
//---------------------------------------------------------------------------
Desktop::~Desktop(void){
//printf("~Desktop()\n");
	if (fDragMessage)
		delete fDragMessage;
	
	void	*ptr;
	for(int32 i=0; (ptr=fRootLayerList.ItemAt(i)); i++){
		delete (RootLayer*)ptr;
	}

	for(int32 i=0; (ptr=fScreenList.ItemAt(i)); i++){
		delete (Screen*)ptr;
	}

	for(int32 i=0; (ptr=fWinBorderList.ItemAt(i)); i++){
		delete (WinBorder*)ptr;
	}
}
//---------------------------------------------------------------------------
void Desktop::Init(void){
	DisplayDriver	*driver = NULL;
	int32			driverCount = 0;
	bool			initDrivers = true;

	while(initDrivers){

#ifdef REAL_MODE
		// If we're using the AccelerantDriver for rendering, eventually we will loop through
		// drivers until one can't initialize in order to support multiple monitors. For now,
		// we'll just load one and be done with it.
		driver		= new AccelerantDriver();
#else
		driver		= new ViewDriver();
#endif

		if(driver->Initialize()){
			driverCount++;

			Screen		*sc = new Screen(driver, BPoint(640, 480), B_RGB32, driverCount);
// TODO: be careful, it may fail to initialize! - Monitor may not support 640x480
			fScreenList.AddItem(sc);

// TODO: remove this when you have a real Driver.
			if (driverCount == 1)//2)
				initDrivers	= false;
		}
		else{
			driver->Shutdown();
			delete	driver;
			driver	= NULL;
			initDrivers	= false;
		}
	}

	if (driverCount < 1){
		delete this;
	}

	InitMode();

	SetActiveRootLayerByIndex(0);
}
//---------------------------------------------------------------------------
void Desktop::InitMode(void){
		// this is init mode for n-SS.
	fActiveScreen	= fScreenList.ItemAt(0)? (Screen*)fScreenList.ItemAt(0): NULL;
	for (int32 i=0; i<fScreenList.CountItems(); i++){
		char		name[32];
		sprintf(name, "RootLayer %ld", i+1);

		Screen		*screens[1];		
		screens[0]	= (Screen*)fScreenList.ItemAt(i);
		
		RootLayer	*rl = new RootLayer(name, 4, this);
		rl->SetScreens(screens, 1, 1);
		
		fRootLayerList.AddItem(rl);
	}
}

				// Methods for multiple monitors.
//---------------------------------------------------------------------------
Screen* Desktop::ScreenAt(int32 index) const{
	Screen	*sc;
	sc		= static_cast<Screen*>(fScreenList.ItemAt(index));

	return sc;
}
//---------------------------------------------------------------------------
int32 Desktop::ScreenCount() const{
	return fScreenList.CountItems();
}
//---------------------------------------------------------------------------
Screen* Desktop::ActiveScreen() const{
	return fActiveScreen;
}
//---------------------------------------------------------------------------
void Desktop::SetActiveRootLayerByIndex(int32 listIndex){
	RootLayer	*rl;
	rl			= RootLayerAt(listIndex);
	if (rl)
		SetActiveRootLayer(rl);
}
//---------------------------------------------------------------------------
void Desktop::SetActiveRootLayer(RootLayer* rl){
	if (fActiveRootLayer == rl)
		return;

	fActiveRootLayer	= rl;

// TODO: fix!!!!!!!!!!!!!!!!!!!!!!!! or not?
		// also set he new front and focus
//	SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());
//	SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());
	
// TODO: other tasks required when this happens. I don't know them now.
// Rebuild & Invalidate
// hide the mouse in the old ActiveRootLayer
// show the mouse in new ActiveRootLayer
}
//---------------------------------------------------------------------------
RootLayer* Desktop::ActiveRootLayer() const{
	return fActiveRootLayer;
}
//---------------------------------------------------------------------------
int32 Desktop::ActiveRootLayerIndex() const{
	int32		rootLayerCount = CountRootLayers(); 
	for(int32 i=0; i<rootLayerCount; i++){
		if(fActiveRootLayer == (RootLayer*)(fRootLayerList.ItemAt(i)))
			return i;
	}
	return -1;
}
//---------------------------------------------------------------------------
RootLayer* Desktop::RootLayerAt(int32 index){
	RootLayer	*rl;
	rl		= static_cast<RootLayer*>(fRootLayerList.ItemAt(index));

	return rl;
}
//---------------------------------------------------------------------------
int32 Desktop::CountRootLayers() const{
	return fRootLayerList.CountItems();
}

DisplayDriver* Desktop::GetDisplayDriver() const{
	return ScreenAt(0)->DDriver();
}
				// Methods for layer(WinBorder) manipulation.
//---------------------------------------------------------------------------
void Desktop::AddWinBorder(WinBorder* winBorder){
	if(fWinBorderList.HasItem(winBorder))
		return;

		// special case for Tracker background window.
	if (winBorder->_level == B_SYSTEM_LAST){
			// it's added in all RottLayers
		for(int32 i=0; i<fRootLayerList.CountItems(); i++){
			((RootLayer*)fRootLayerList.ItemAt(i))->AddWinBorder(winBorder);
		}
	}
		// other windows are added to the current RootLayer only.
	else
		ActiveRootLayer()->AddWinBorder(winBorder);

		// add that pointer to user winboder list so that we can keep track of them.		
	fLayerLock.Lock();
	fWinBorderList.AddItem(winBorder);
	fLayerLock.Unlock();
	
// TODO: remove those 2? I vote for: YES! still... have to think...
//	SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());
//	SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());
}
//---------------------------------------------------------------------------
void Desktop::RemoveWinBorder(WinBorder* winBorder){
	if(winBorder->_level == B_SYSTEM_LAST){
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->RemoveWinBorder(winBorder);
	}
	else{
		winBorder->GetRootLayer()->RemoveWinBorder(winBorder);
	}

// TODO: remove those 4? I vote for: YES! still... have to think...
//	if (winBorder == fFrontWinBorder)
//		SetFrontWinBorder(fActiveRootLayer->ActiveWorkspace()->FrontLayer());
//	if (winBorder == fFocusWinBorder)
//		SetFocusWinBorder(fActiveRootLayer->ActiveWorkspace()->FocusLayer());

	fLayerLock.Lock();
	fWinBorderList.RemoveItem(winBorder);
	fLayerLock.Unlock();
}
//---------------------------------------------------------------------------
bool Desktop::HasWinBorder(WinBorder* winBorder){
	return fWinBorderList.HasItem(winBorder);
}
//---------------------------------------------------------------------------
void Desktop::SetFrontWinBorder(WinBorder* winBorder){
	fFrontWinBorder		= winBorder;
// TODO: implement
}
//---------------------------------------------------------------------------
// TODO: remove shortly?
void Desktop::SetFoooocusWinBorder(WinBorder* winBorder){
	if (FocusWinBorder() == winBorder && (winBorder && !winBorder->IsHidden()))
		return;

	fFocusWinBorder		= FocusWinBorder();

	// NOTE: we assume both, the old and new focus layer are in the active workspace
	WinBorder	*newFocus = NULL;

	if(fFocusWinBorder){
		fFocusWinBorder->SetFocus(false);
	}
	
	if(winBorder){
// TODO: NO! this call is to determine the correct order! NOT to rebuild/redraw anything!
// TODO: WinBorder::SetFront... will do that - both!
// TODO: modify later
// TODO: same applies for the focus state - RootLayer::SetFocus also does redraw
//	Workspace::SetFocus - Only determines the focus! Just like above!
/*
		newFocus	= winBorder->GetRootLayer()->ActiveWorkspace()->SetFocusLayer(winBorder);
		newFocus->SetFocus(true);
*/
		Workspace		*aws;

		aws				= winBorder->GetRootLayer()->ActiveWorkspace();
		aws->SearchAndSetNewFocus(winBorder);
		
			//why do put this line? Eh... I will remove it later...
		newFocus		= aws->FocusLayer();

		aws->FocusLayer()->SetFocus(true);
		
		aws->Invalidate();
	}

	fFocusWinBorder		= newFocus;
}
//---------------------------------------------------------------------------
WinBorder* Desktop::FrontWinBorder(void) const{
	return fActiveRootLayer->ActiveWorkspace()->FrontLayer();
//	return fFrontWinBorder;
}
//---------------------------------------------------------------------------
WinBorder* Desktop::FocusWinBorder(void) const{
	return fActiveRootLayer->ActiveWorkspace()->FocusLayer();
//	return fFocusWinBorder;
}

				// Input related methods
//---------------------------------------------------------------------------
void Desktop::MouseEventHandler(PortMessage *msg){
	switch(msg->Code()){
		case B_MOUSE_DOWN:{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

			BPoint		pt;
			int64		dummy;
			msg->Read<int64>(&dummy);
			msg->Read<float>(&pt.x);
			msg->Read<float>(&pt.y);

			printf("MOUSE DOWN: at (%f, %f)\n", pt.x, pt.y);
			
			WinBorder	*target;
			RootLayer	*rl;
			Workspace	*ws;
			rl			= ActiveRootLayer();
			ws			= rl->ActiveWorkspace();
			target		= ws->SearchLayerUnderPoint(pt);
			if (target){
				fGeneralLock.Lock();
				rl->fMainLock.Lock();

				ws->SearchAndSetNewFront(target);
				ws->SetFocusLayer(target);

				rl->fMainLock.Unlock();
				fGeneralLock.Unlock();
			}
			break;
		}
		case B_MOUSE_UP:{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

			BPoint		pt;
			int64		dummy;
			msg->Read<int64>(&dummy);
			msg->Read<float>(&pt.x);
			msg->Read<float>(&pt.y);

			printf("MOUSE UP: at (%f, %f)\n", pt.x, pt.y);

			break;
		}
		case B_MOUSE_MOVED:{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
			int64 dummy;
			float x,y;
			msg->Read<int64>(&dummy);
			msg->Read<float>(&x);
			msg->Read<float>(&y);
			
			// We need this so that we can see the cursor on the screen
			if(fActiveScreen)
				fActiveScreen->DDriver()->MoveCursorTo(x,y);
			break;
		}
		case B_MOUSE_WHEEL_CHANGED:{
			break;
		}
		default:{
			printf("\nDesktop::MouseEventHandler(): WARNING: unknown message\n\n");
		}
	}
}
//---------------------------------------------------------------------------
void Desktop::KeyboardEventHandler(PortMessage *msg){
}
//---------------------------------------------------------------------------
void Desktop::SetDragMessage(BMessage* msg){
	if (fDragMessage){
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage	= new BMessage(*msg);
}
//---------------------------------------------------------------------------
BMessage* Desktop::DragMessage(void) const{
	return fDragMessage;
}

				// Methods for various desktop stuff handled by the server
//---------------------------------------------------------------------------
void Desktop::SetScrollBarInfo(const scroll_bar_info &info){
	fScrollBarInfo	= info;
}
//---------------------------------------------------------------------------
scroll_bar_info Desktop::ScrollBarInfo(void) const{
	return fScrollBarInfo;
}
//---------------------------------------------------------------------------
void Desktop::SetMenuInfo(const menu_info &info){
	fMenuInfo	= info;
}
//---------------------------------------------------------------------------
menu_info Desktop::MenuInfo(void) const{
	return fMenuInfo;
}
//---------------------------------------------------------------------------
void Desktop::UseFFMouse(const bool &useffm){
	fFFMouseMode	= useffm;
}
//---------------------------------------------------------------------------
bool Desktop::FFMouseInUse(void) const{
	return fFFMouseMode;
}
//---------------------------------------------------------------------------
void Desktop::SetFFMouseMode(const mode_mouse &value){
	fMouseMode	= value;
}
//---------------------------------------------------------------------------
mode_mouse Desktop::FFMouseMode(void) const{
	return fMouseMode;
}
//---------------------------------------------------------------------------
bool Desktop::ReadWorkspaceData(void){
// TODO: implement
	return true;
}
//---------------------------------------------------------------------------
void Desktop::SaveWorkspaceData(void){
// TODO: implement
}
//---------------------------------------------------------------------------
void Desktop::RemoveSubsetWindow(WinBorder* wb){
	WinBorder		*winBorder = NULL;
	int32			count = fWinBorderList.CountItems();
	for(int32 i=0; i < count; i++){
		winBorder	= static_cast<WinBorder*>(fWinBorderList.ItemAt(i));
		if (winBorder->_level == B_NORMAL_FEEL)
			winBorder->Window()->fWinFMWList.RemoveItem(wb);
	}
	
	RootLayer		*rl = winBorder->GetRootLayer();
	int32			countWKs = rl->WorkspaceCount();
	for (int32 i=0; i < countWKs; i++){
		rl->WorkspaceAt(i+1)->RemoveLayerPtr(wb);
	}
}
//---------------------------------------------------------------------------
void Desktop::PrintToStream(){
	printf("RootLayer List:\n=======\n");
	for(int32 i=0; i<fRootLayerList.CountItems(); i++){
		printf("\t%s\n", ((RootLayer*)fRootLayerList.ItemAt(i))->GetName());
		((RootLayer*)fRootLayerList.ItemAt(i))->PrintToStream();
		printf("-------\n");
	}
	printf("=======\nActive RootLayer: %s\n", fActiveRootLayer? fActiveRootLayer->GetName(): "NULL");
//	printf("Active WinBorder: %s\n", fActiveWinBorder? fActiveWinBorder->Name(): "NULL");
	
	printf("Screen List:\n");
	for(int32 i=0; i<fScreenList.CountItems(); i++){
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
	}
	
}
//---------------------------------------------------------------------------
WinBorder* Desktop::FindWinBorderByServerWindowTokenAndTeamID(int32 token, team_id teamID){
	WinBorder*		wb;
	fLayerLock.Lock();
	for (int32 i = 0; (wb = (WinBorder*)fWinBorderList.ItemAt(i)); i++){
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeamID() == teamID)
			break;
	}
	fLayerLock.Unlock();
	
	return wb;
}
//---------------------------------------------------------------------------
void Desktop::PrintVisibleInRootLayerNo(int32 no){
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
