
#include <stdio.h>
#include <Window.h>
#include <List.h>

#include "ServerApp.h"
#include "Globals.h"
#include "RootLayer.h"
#include "Layer.h"
#include "Workspace.h"
#include "ServerScreen.h"
#include "WinBorder.h"
#include "ServerWindow.h"
#include "Desktop.h"
#include "FMWList.h"

//---------------------------------------------------------------------------
RootLayer::RootLayer(const char *layername, int32 workspaceCount, Desktop *desktop)
	: Layer(BRect(0,0,0,0), layername, 0, B_NORMAL_WINDOW_FEEL, 0, NULL)
{
	fDesktop			= desktop;
	fActiveWorkspace	= NULL;
	fRows				= 0;
	fColumns			= 0;
// TODO: read these 3 from a configuration file.
	fScreenXResolution	= 0;
	fScreenYResolution	= 0;
	fColorSpace			= B_RGB32;

	_view_token			= 0; // is this used for WinBorders?
	_hidden				= false;
	
	SetWorkspaceCount(workspaceCount);
}
//---------------------------------------------------------------------------
RootLayer::~RootLayer(){
//printf("~RootLayer( %s )\n", Name());
	// RootLayer object just uses Screen objects, it is not allowed to delete them.
}
//---------------------------------------------------------------------------
void RootLayer::Draw(const BRect &r){
/* THIS method is here because of DisplayDriver testing!
 *	It SHOULD BE REMOVED as soon as testing is done!!!!
 */
/*
#ifdef DISPLAYDRIVER_TEST_HACK
	int8 pattern[8];
	int8 pattern2[8];
	memset(pattern,255,8);
	memset(pattern2,128+64+32+16,8);
	BRect r1(100,100,1500,1100);
	BPoint pts[4];
	pts[0].x = 200;
	pts[0].y = 200;
	pts[1].x = 400;
	pts[1].y = 1000;
	pts[2].x = 600;
	pts[2].y = 400;
	pts[3].x = 1200;
	pts[3].y = 800;
	BPoint triangle[3];
	BRect triangleRect(100,100,400,300);
	triangle[0].x = 100;
	triangle[0].y = 100;
	triangle[1].x = 100;
	triangle[1].y = 300;
	triangle[2].x = 400;
	triangle[2].y = 300;
	BPoint polygon[6];
	BRect polygonRect(100,100,300,400);
	polygon[0].x = 100;
	polygon[0].y = 100;
	polygon[1].x = 100;
	polygon[1].y = 400;
	polygon[2].x = 200;
	polygon[2].y = 300;
	polygon[3].x = 300;
	polygon[3].y = 400;
	polygon[4].x = 300;
	polygon[4].y = 100;
	polygon[5].x = 200;
	polygon[5].y = 200;

	_layerdata->highcolor.SetColor(255,0,0,255);
	_layerdata->lowcolor.SetColor(255,255,255,255);
	_driver->FillRect(r1,_layerdata,pattern);

	_layerdata->highcolor.SetColor(255,255,0,255);
	_driver->StrokeLine(BPoint(100,100),BPoint(1500,1100),_layerdata,pattern);

	_layerdata->highcolor.SetColor(0,0,255,255);
	_driver->StrokeBezier(pts,_layerdata,pattern);
	_driver->StrokeArc(BRect(200,300,400,600),30,270,_layerdata,pattern);
	_driver->StrokeEllipse(BRect(200,700,400,900),_layerdata,pattern);
	_driver->StrokeRect(BRect(650,1000,750,1090),_layerdata,pattern);
	_driver->StrokeRoundRect(BRect(200,1000,600,1090),30,40,_layerdata,pattern);
//	_driver->StrokePolygon(polygon,6,polygonRect,_layerdata,pattern);
//	_driver->StrokeTriangle(triangle,triangleRect,_layerdata,pattern);
	_layerdata->highcolor.SetColor(255,0,255,255);
	_driver->FillArc(BRect(1250,300,1450,600),30,270,_layerdata,pattern);
//	_driver->FillBezier(pts,_layerdata,pattern);
	_driver->FillEllipse(BRect(800,300,1200,600),_layerdata,pattern);
	_driver->FillRoundRect(BRect(800,1000,1200,1090),30,40,_layerdata,pattern2);
	_driver->FillPolygon(polygon,6,polygonRect,_layerdata,pattern);
//	_driver->FillTriangle(triangle,triangleRect,_layerdata,pattern);
#endif
*/
}
//---------------------------------------------------------------------------
void RootLayer::MoveBy(float x, float y){
}
//---------------------------------------------------------------------------
void RootLayer::ResizeBy(float x, float y){
}
//---------------------------------------------------------------------------
Layer* RootLayer::VirtualTopChild() const{
	return fActiveWorkspace->GoToTopItem();
}
//---------------------------------------------------------------------------
Layer* RootLayer::VirtualLowerSibling() const{
	return fActiveWorkspace->GoToLowerItem();
}
//---------------------------------------------------------------------------
Layer* RootLayer::VirtualUpperSibling() const{
	return fActiveWorkspace->GoToUpperItem();
}
//---------------------------------------------------------------------------
Layer* RootLayer::VirtualBottomChild() const{
	return fActiveWorkspace->GoToBottomItem();
}
//---------------------------------------------------------------------------
void RootLayer::AddWinBorderToWorkspaces(WinBorder* winBorder, uint32 wks){
	if (wks == B_CURRENT_WORKSPACE){
		ActiveWorkspace()->AddLayerPtr(winBorder);
		return;
	}

	for( int32 i=0; i < 32; i++){
		if( wks & (0x00000001 << i) && i < WorkspaceCount()){
			WorkspaceAt(i+1)->AddLayerPtr(winBorder);
		}
	}
}
/* The main job, beside adding winBorder as a child, is to determine
 *	in witch workspaces to add this window.
 */
//---------------------------------------------------------------------------
void RootLayer::AddWinBorder(WinBorder* winBorder){
		// add winBorder to the known list of WinBorders so we can keep track of it.
	AddChild(winBorder, this);

		// add winBorder to the desired workspaces
	switch(winBorder->Window()->GetFeel()){
		case B_MODAL_SUBSET_WINDOW_FEEL:{
			/* this kind of window isn't added anywhere. It *will* be added
				to main window's subset when winBorder::AddToSubsetOf(main)
				will be called.*/
			break;
		}
			
		case B_MODAL_APP_WINDOW_FEEL:{
				// add to ***app's*** list of Floating/Modal Windows.
			winBorder->Window()->GetApp()->GetFMWList()->AddItem(winBorder);

				// determine in witch workspaces to add this winBorder.
			uint32		wks = 0;
// TODO: change later when you put this code into the server
			for (int32 i=0; i<WorkspaceCount(); i++){
				Workspace	*ws = WorkspaceAt(i+1);
				for (WinBorder *wb = ws->GoToBottomItem(); wb!=NULL; wb = ws->GoToUpperItem()){
					if ( !(wb->IsHidden()) &&
						 winBorder->Window()->GetClientTeamID() == wb->Window()->GetClientTeamID())
					{
						wks = wks | winBorder->Window()->GetWorkspaces();
						break;
					}
				}
			}
// by using _bottomchild and _uppersibling.

			AddWinBorderToWorkspaces(winBorder, wks);
			break;
		}
				
		case B_MODAL_ALL_WINDOW_FEEL:{
				// add to ***system's*** list of Floating/Modal Windows.
			fMainFMWList.AddItem(winBorder);
				// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}

		case B_FLOATING_SUBSET_WINDOW_FEEL:{
			/* this kind of window isn't added anywhere. It *will* be added to WS's list
				when its main window will become the front one.
				Also, it will be added to MainWinBorder's list when
				winBorder::AddToSubset(main) is called.*/
			break;
		}
				
		case B_FLOATING_APP_WINDOW_FEEL:{
				// add to ***app's*** list of Floating/Modal Windows.
			winBorder->Window()->GetApp()->GetFMWList()->AddItem(winBorder);
			
			for (int32 i=0; i<WorkspaceCount(); i++){
				Workspace	*ws = WorkspaceAt(i+1);
				WinBorder	*wb = ws->FrontLayer();
				if(wb && wb->Window()->GetClientTeamID() == winBorder->Window()->GetClientTeamID()
					&& wb->Window()->GetFeel() == B_NORMAL_WINDOW_FEEL)
				{
					ws->AddLayerPtr(winBorder);
				}
			}

			break;
		}
				
		case B_FLOATING_ALL_WINDOW_FEEL:{
				// add to ***system's*** list of Floating/Modal Windows.
			fMainFMWList.AddItem(winBorder);
				// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		
		case B_NORMAL_WINDOW_FEEL:{
				// add this winBorder to the specified workspaces
			AddWinBorderToWorkspaces(winBorder, winBorder->Window()->GetWorkspaces());
			break;
		}
		
		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:{
				// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		default:
			break;
	}
}
/* This method does 3 things:
 *	1) Removes MODAL/SUBSET windows from system/app/window subset list.
 *	2) Removes this window from any workspace it appears in.
 *	3) Removes this window from RootLayer's list of chilren.
 */
//---------------------------------------------------------------------------
void RootLayer::RemoveWinBorder(WinBorder* winBorder){

	int32		feel = winBorder->Window()->GetFeel();
	if(feel == B_MODAL_SUBSET_WINDOW_FEEL || feel == B_FLOATING_SUBSET_WINDOW_FEEL){
		desktop->RemoveSubsetWindow(winBorder);
	}
	else if (feel == B_MODAL_APP_WINDOW_FEEL || feel == B_FLOATING_APP_WINDOW_FEEL){
		RemoveAppWindow(winBorder);
	}
	else if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL
			|| feel == B_SYSTEM_FIRST || feel == B_SYSTEM_LAST)
	{
		if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL)
			fMainFMWList.RemoveItem(winBorder);

		int32		count = WorkspaceCount();
		for(int32 i=0; i < count; i++)
			WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
	}
	else{ // for B_NORMAL_WINDOW_FEEL
		uint32		workspaces	= winBorder->Window()->GetWorkspaces();
		int32		count		= WorkspaceCount();
		for( int32 i=0; i < 32 && i < count; i++)
			if( workspaces & (0x00000001 << i))
					WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
	}

	RemoveChild(winBorder);
}
//---------------------------------------------------------------------------
void RootLayer::ChangeWorkspacesFor(WinBorder* winBorder, uint32 newWorkspaces){
		// only NORMAL windows are affected by this change
	if(!winBorder->_level != B_NORMAL_FEEL)
		return;

	uint32		oldWorkspaces = winBorder->Window()->GetWorkspaces();
	for(int32 i=0; i < WorkspaceCount(); i++){
		if ((oldWorkspaces & (0x00000001 << i)) && (newWorkspaces & (0x00000001 << i))){
			// do nothing.
		}
		else if(oldWorkspaces & (0x00000001 << i)){
			WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
		}
		else if (newWorkspaces & (0x00000001 << i)){
			WorkspaceAt(i+1)->AddLayerPtr(winBorder);
		}
	}
}
//---------------------------------------------------------------------------
void RootLayer::SetScreens(Screen *screen[], int32 rows, int32 columns){
// NOTE: *All* Screens *MUST* have the same resolution.
	fScreenPtrList.MakeEmpty();
	BRect	newFrame(0, 0, 0, 0);
	for (int32 i=0; i < rows; i++){
		if (i==0)
		for(int32 j=0; j < columns; j++){
			fScreenPtrList.AddItem(screen[i*rows + j]);

			newFrame.right		+= screen[i*rows + j]->Resolution().x;
		}
		newFrame.bottom		+= screen[i*rows]->Resolution().y;
	}

	newFrame.right	-= 1;
	newFrame.bottom	-= 1;

	_frame		= newFrame;
	fRows		= rows;
	fColumns	= columns;
	fScreenXResolution	= (int32)(screen[0]->Resolution().x);
	fScreenYResolution	= (int32)(screen[0]->Resolution().y);
	
//	RebuildRegions(Frame());
//	Invalidate(Frame());
}
//---------------------------------------------------------------------------
Screen** RootLayer::Screens(){
	return (Screen**)fScreenPtrList.Items();
}
//---------------------------------------------------------------------------
bool RootLayer::SetScreenResolution(int32 width, int32 height, uint32 colorspace){
		// if there is no need to change resolution... exit
	if (fScreenXResolution == width && fScreenYResolution == height &&
		fColorSpace == colorspace)
		return false;

	bool		accepted = true;

	for (int i=0; i < fScreenPtrList.CountItems(); i++){
		Screen		*screen;
		screen		= static_cast<Screen*>(fScreenPtrList.ItemAt(i));
		
		if ( !(screen->SupportsResolution(BPoint(width, height), colorspace)) )
			accepted	= false;
	}
	
	if (accepted){
		for (int i=0; i < fScreenPtrList.CountItems(); i++){
			Screen		*screen;
			screen		= static_cast<Screen*>(fScreenPtrList.ItemAt(i));

			screen->SetResolution(BPoint(width, height), colorspace);
		}

		Screen		**screens = (Screen**)fScreenPtrList.Items();
		SetScreens(screens, fRows, fColumns);

		return true;
	}
	
	return false;
}
//---------------------------------------------------------------------------
void RootLayer::SetWorkspaceCount(const int32 count){
	if ((count < 1 && count > 32) || count == WorkspaceCount())
		return;
	
	int32		exActiveWorkspaceIndex = ActiveWorkspaceIndex();

	BList		newWSPtrList;
	void		*workspacePtr;

	for(int i=0; i < count; i++){
		workspacePtr	= fWSPtrList.ItemAt(i);
		if (workspacePtr){
			newWSPtrList.AddItem(workspacePtr);
		}
		else{
			Workspace	*ws;
			ws			= new Workspace(fColorSpace, i+1, BGColor());
			newWSPtrList.AddItem(ws);
		}
	}
	
		// delete other Workspace objects if WS count is smaller than current one.
	for (int j=count; j < fWSPtrList.CountItems(); j++){
		Workspace	*ws = NULL;
		ws			= static_cast<Workspace*>(fWSPtrList.ItemAt(j));
		if (ws){
			delete ws;
		}
		else{
			printf("\nSERVER: PANIC: in RootLayer::SetWorkspaceCount()\n\n");
			return;
		}
	}
	
	fWSPtrList		= newWSPtrList;
	if (exActiveWorkspaceIndex > count)
		SetActiveWorkspaceByIndex(count); 

		// if true, this is the first time this method is called.
	if (exActiveWorkspaceIndex == -1)
		SetActiveWorkspaceByIndex(1); 		
}
//---------------------------------------------------------------------------
int32 RootLayer::WorkspaceCount() const{
	return fWSPtrList.CountItems();
}
//---------------------------------------------------------------------------
Workspace* RootLayer::WorkspaceAt(const int32 index) const{
	Workspace		*ws = NULL;
	ws				= static_cast<Workspace*>(fWSPtrList.ItemAt(index-1));

	return ws;
}
//---------------------------------------------------------------------------
void RootLayer::SetActiveWorkspaceByIndex(const int32 index){
	Workspace		*ws = NULL;
	ws				= static_cast<Workspace*>(fWSPtrList.ItemAt(index-1));
	if (ws)
		SetActiveWorkspace(ws);
}
//---------------------------------------------------------------------------
void RootLayer::SetActiveWorkspace(Workspace *ws){
	if (fActiveWorkspace == ws || !ws)
		return;

	fActiveWorkspace	= ws;

//	RebuildRegions(Frame());
//	Invalidate(Frame());
}
//---------------------------------------------------------------------------
int32 RootLayer::ActiveWorkspaceIndex() const{
	if (fActiveWorkspace)
		return fActiveWorkspace->ID();
	else
		return -1;
}
//---------------------------------------------------------------------------
Workspace* RootLayer::ActiveWorkspace() const{
	return fActiveWorkspace;
}
//---------------------------------------------------------------------------
void RootLayer::SetBGColor(const RGBColor &col){
	ActiveWorkspace()->SetBGColor(col);
	
	_layerdata->viewcolor	= col;

//	Invalidate(Frame());
}
//---------------------------------------------------------------------------
RGBColor RootLayer::BGColor(void) const{	
	return _layerdata->viewcolor;
}
//---------------------------------------------------------------------------
void RootLayer::RemoveAppWindow(WinBorder *wb){
	wb->Window()->GetApp()->GetFMWList()->RemoveItem(wb);

	int32		count = WorkspaceCount();
	for(int32 i=0; i < count; i++){
		WorkspaceAt(i+1)->RemoveLayerPtr(wb);
	}
}
//---------------------------------------------------------------------------
void RootLayer::PrintToStream(){
	printf("\nRootLayer '%s' internals:\n", GetName());
	printf("Screen list:\n");
	for(int32 i=0; i<fScreenPtrList.CountItems(); i++){
		printf("\t%ld\n", ((Screen*)fScreenPtrList.ItemAt(i))->ScreenNumber());
	}
	printf("Screen rows: %ld\nScreen columns: %ld\n", fRows, fColumns);
	printf("Resolution for all Screens: %ldx%ldx%ld\n", fScreenXResolution, fScreenYResolution, fColorSpace);
	printf("Workspace list:\n");
	for(int32 i=0; i<fWSPtrList.CountItems(); i++){
		printf("\t~~~Workspace: %ld\n", ((Workspace*)fWSPtrList.ItemAt(i))->ID());
		((Workspace*)fWSPtrList.ItemAt(i))->PrintToStream();
		printf("~~~~~~~~\n");
	}
	printf("Active Workspace: %ld\n", fActiveWorkspace? fActiveWorkspace->ID(): -1);

}
