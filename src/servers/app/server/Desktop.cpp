//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Desktop.cpp
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------
#include <stdio.h>
#include <Entry.h>
#include <Region.h>
#include <Message.h>

#include "AccelerantDriver.h"
#include "AppServer.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "Globals.h"
#include "Layer.h"
#include "PortMessage.h"
#include "RootLayer.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "ViewDriver.h"
#include "DirectDriver.h"
#include "WinBorder.h"
#include "Workspace.h"

//#define DEBUG_DESKTOP
#define DEBUG_KEYHANDLING

#ifdef DEBUG_DESKTOP
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

Desktop::Desktop(void)
{
	fDragMessage		= NULL;
	fActiveRootLayer	= NULL;
	fMouseTarget		= NULL;
	fActiveScreen		= NULL;
	fScreenShotIndex	= 1;
}

Desktop::~Desktop(void)
{
	if (fDragMessage)
		delete fDragMessage;
	
	void	*ptr;
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

		if(DISPLAYDRIVER == HWDRIVER)
		{
			// If we're using the AccelerantDriver for rendering, eventually we will loop through
			// drivers until one can't initialize in order to support multiple monitors. For now,
			// we'll just load one and be done with it.
			driver = new AccelerantDriver();
		}
		else if(DISPLAYDRIVER == DIRECTDRIVER)
		{
			// Eventually, it would be nice to do away with ViewDriver and replace it with
			// one which uses a double-buffered BDirectWindow as a rendering context
			driver = new DirectDriver();
		}
		else
		{
			// Eventually, it would be nice to do away with ViewDriver and replace it with
			// one which uses a double-buffered BDirectWindow as a rendering context
			driver = new ViewDriver();
		}

		if(driver->Initialize())
		{
			driverCount++;

			Screen		*sc = new Screen(driver, BPoint(640, 480), B_RGB32, driverCount);

			// TODO: be careful, it may fail to initialize! - Monitor may not support 640x480
			fScreenList.AddItem(sc);

			// TODO: remove this when you have a real Driver.
			if (driverCount == 1)
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

	if (driverCount < 1){
		delete this;
		return;
	}
	
	InitMode();

	SetActiveRootLayerByIndex(0);
}

void Desktop::InitMode(void)
{
	// this is init mode for n-SS.
	fActiveScreen	= fScreenList.ItemAt(0)? (Screen*)fScreenList.ItemAt(0): NULL;
	for (int32 i=0; i<fScreenList.CountItems(); i++)
	{
		char		name[32];
		sprintf(name, "RootLayer %ld", i+1);

		Screen		*screens[1];		
		screens[0]	= (Screen*)fScreenList.ItemAt(i);
		
		RootLayer	*rl = new RootLayer(name, 4, this, GetDisplayDriver());
		rl->SetScreens(screens, 1, 1);
		rl->RebuildFullRegion();
		
		fRootLayerList.AddItem(rl);
	}
}

//---------------------------------------------------------------------------
//					Methods for multiple monitors.
//---------------------------------------------------------------------------


Screen* Desktop::ScreenAt(int32 index) const
{
	Screen	*sc= static_cast<Screen*>(fScreenList.ItemAt(index));

	return sc;
}

int32 Desktop::ScreenCount(void) const
{
	return fScreenList.CountItems();
}

Screen* Desktop::ActiveScreen(void) const
{
	return fActiveScreen;
}

void Desktop::SetActiveRootLayerByIndex(int32 listIndex)
{
	RootLayer	*rl=RootLayerAt(listIndex);

	if (rl)
		SetActiveRootLayer(rl);
}

void Desktop::SetActiveRootLayer(RootLayer* rl)
{
	if (fActiveRootLayer == rl)
		return;

	fActiveRootLayer	= rl;
	
// TODO:
// hide the mouse in the old ActiveRootLayer
// show the mouse in new ActiveRootLayer
	fActiveRootLayer->FullInvalidate(fActiveRootLayer->Bounds());
}

RootLayer* Desktop::ActiveRootLayer(void) const
{
	return fActiveRootLayer;
}

int32 Desktop::ActiveRootLayerIndex(void) const
{
	int32		rootLayerCount = CountRootLayers(); 
	for(int32 i=0; i<rootLayerCount; i++)
	{
		if(fActiveRootLayer == (RootLayer*)(fRootLayerList.ItemAt(i)))
			return i;
	}
	return -1;
}

RootLayer* Desktop::RootLayerAt(int32 index)
{
	RootLayer	*rl=static_cast<RootLayer*>(fRootLayerList.ItemAt(index));

	return rl;
}

int32 Desktop::CountRootLayers() const
{
	return fRootLayerList.CountItems();
}

DisplayDriver* Desktop::GetDisplayDriver() const
{
	return ScreenAt(0)->DDriver();
}

//---------------------------------------------------------------------------
//				Methods for layer(WinBorder) manipulation.
//---------------------------------------------------------------------------


void Desktop::AddWinBorder(WinBorder* winBorder)
{
	desktop->fGeneralLock.Lock();

	if(fWinBorderList.HasItem(winBorder))
		return;

	// special case for Tracker background window.
	if (winBorder->fLevel == B_SYSTEM_LAST)
	{
		// it's added in all RottLayers
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->AddWinBorder(winBorder);
	}
	else
		// other windows are added to the current RootLayer only.
		ActiveRootLayer()->AddWinBorder(winBorder);

	// add that pointer to user winboder list so that we can keep track of them.		
	fLayerLock.Lock();
	fWinBorderList.AddItem(winBorder);
	fLayerLock.Unlock();

	desktop->fGeneralLock.Unlock();
}

void Desktop::RemoveWinBorder(WinBorder* winBorder)
{
	desktop->fGeneralLock.Lock();

	if(winBorder->fLevel == B_SYSTEM_LAST)
	{
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->RemoveWinBorder(winBorder);
	}
	else
		winBorder->GetRootLayer()->RemoveWinBorder(winBorder);

	fLayerLock.Lock();
	fWinBorderList.RemoveItem(winBorder);
	fLayerLock.Unlock();

	desktop->fGeneralLock.Unlock();
}

bool Desktop::HasWinBorder(WinBorder* winBorder)
{
	return fWinBorderList.HasItem(winBorder);
}

//---------------------------------------------------------------------------
//				Input related methods
//---------------------------------------------------------------------------
void Desktop::MouseEventHandler(PortMessage *msg)
{
	// TODO: locking mechanism needs SERIOUS rethought
	switch(msg->Code())
	{
		case B_MOUSE_DOWN:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

			BPoint pt;
			int64 dummy;
			
			msg->Read<int64>(&dummy);
			msg->Read<float>(&pt.x);
			msg->Read<float>(&pt.y);
			
			// After we read the data, we need to reset it for MouseDown()
			msg->Rewind();

			// printf("MOUSE DOWN: at (%f, %f)\n", pt.x, pt.y);
			
			WinBorder	*target;
			RootLayer	*rl;
			Workspace	*ws;
			rl			= ActiveRootLayer();
			ws			= rl->ActiveWorkspace();
			target		= ws->SearchWinBorder(pt);
			if (target)
			{
				fGeneralLock.Lock();
				rl->fMainLock.Lock();
#if 0
printf("Target: %s\n", target->GetName());
printf("Front: %s\n", ws->FrontLayer()->GetName());
printf("Focus: %s\n", ws->FocusLayer()->GetName());
#endif
				if (target != ws->FrontLayer())
				{
					WinBorder		*previousFocus;
					WinBorder		*activeFocus;
					BRegion			invalidRegion;

					ws->BringToFrontANormalWindow(target);
					ws->SearchAndSetNewFront(target);
					previousFocus	= ws->FocusLayer();
					activeFocus		= ws->SetFocusLayer(target);

					activeFocus->Window()->Lock();

					if (target == activeFocus && target->Window()->Flags() & B_WILL_ACCEPT_FIRST_CLICK)
						target->MouseDown(msg, true);
					else
						target->MouseDown(msg, false);

					// may be or may be empty.
// TODO: what if modal of floating windows are in front of us?
					invalidRegion.Include(&(activeFocus->fFull));
					invalidRegion.Include(&(activeFocus->fTopLayer->fFull));
					activeFocus->fParent->RebuildAndForceRedraw(invalidRegion, activeFocus);

					if (previousFocus != activeFocus && previousFocus)
					{
						if (previousFocus->fVisible.CountRects() > 0)
						{
							invalidRegion.MakeEmpty();
							invalidRegion.Include(&(previousFocus->fVisible));
							activeFocus->fParent->Invalidate(invalidRegion);
						}
					}

					fMouseTarget = target;
#if 0
printf("2Target: %s\n", target->GetName());
printf("2Front: %s\n", ws->FrontLayer()->GetName());
printf("2Focus: %s\n", ws->FocusLayer()->GetName());
#endif

					activeFocus->Window()->Unlock();
				}
				else // target == ws->FrontLayer()
				{
					// only if target has the focus!
					if (target == ws->FocusLayer())
					{
						target->Window()->Lock();
						target->MouseDown(msg, true);
						target->Window()->Unlock();
					}
				}

				rl->fMainLock.Unlock();
				fGeneralLock.Unlock();
			}
			else // target == NULL
			{
			}
			break;
		}
		case B_MOUSE_UP:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

			if (fMouseTarget)
			{
				fMouseTarget->Window()->Lock();
				fMouseTarget->MouseUp(msg);
				fMouseTarget->Window()->Unlock();

				fMouseTarget = NULL;				
			}
			else
			{
				BPoint pt;
				int64 dummy;
				int32 mod;

				msg->Read<int64>(&dummy);
				msg->Read<float>(&pt.x);
				msg->Read<float>(&pt.y);
				msg->Read<int32>(&mod);

				// After we read the data, we need to reset it for MouseUp()
				msg->Rewind();

				WinBorder *target = ActiveRootLayer()->ActiveWorkspace()->SearchWinBorder(pt);
				if(target){
					target->Window()->Lock();
					target->MouseUp(msg);
					target->Window()->Unlock();
				}
			}
			
			// printf("MOUSE UP: at (%f, %f)\n", pt.x, pt.y);

			break;
		}
		case B_MOUSE_MOVED:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
			int64 dummy;
			float x,y;
			int32 buttons;
			
			msg->Read<int64>(&dummy);
			msg->Read<float>(&x);
			msg->Read<float>(&y);
			msg->Read<int32>(&buttons);

			// After we read the data, we need to reset it for MouseDown()
			msg->Rewind();

			if (fMouseTarget)
			{
				fActiveScreen->DDriver()->HideCursor();
				fActiveScreen->DDriver()->MoveCursorTo(x,y);

				fMouseTarget->Window()->Lock();
				fMouseTarget->MouseMoved(msg);
				fMouseTarget->Window()->Unlock();

				fActiveScreen->DDriver()->ShowCursor();
			}
			else
			{
				WinBorder *target = ActiveRootLayer()->ActiveWorkspace()->SearchWinBorder(BPoint(x,y));
				if(target){
					target->Window()->Lock();
					target->MouseMoved(msg);
					target->Window()->Unlock();
				}

				fActiveScreen->DDriver()->MoveCursorTo(x,y);
			}

			break;
		}
		case B_MOUSE_WHEEL_CHANGED:
		{
			// TODO: Pass this on to the client ServerWindow
			break;
		}
		default:
		{
			printf("\nDesktop::MouseEventHandler(): WARNING: unknown message\n\n");
			break;
		}
	}
}

void Desktop::KeyboardEventHandler(PortMessage *msg)
{
	int8 *index=(int8*)msg->Buffer();
	
	switch(msg->Code())
	{
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) int8 number of bytes to follow containing the 
			//		generated string
			// 8) Character string generated by the keystroke
			// 9) int8[16] state of all keys
			
			// Obtain only what data which we'll need
			STRACE(("Key Down: 0x%lx\n",scancode));
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int32) + (sizeof(int8) * 3);
			int8 stringlength=*index; index+=stringlength;
			if(DISPLAYDRIVER==HWDRIVER)
			{
				// Check for workspace change or safe video mode
				if(scancode>0x01 && scancode<0x0e)
				{
					if(scancode==0x0d)
					{
						if(modifiers & (B_LEFT_COMMAND_KEY |
							B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY))
						{
							// TODO: Set to Safe Mode here. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
				}
			
				if(modifiers & B_CONTROL_KEY)
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					
					//TODO: change		
					//SetWorkspace(scancode-2);
					break;
				}	

				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}

				// PrintScreen
				if(scancode==0xe)
				{
					if(GetDisplayDriver())
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							fScreenShotIndex++;
							sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						}
						fScreenShotIndex++;
						GetDisplayDriver()->DumpToFile(filename);
						break;
					}
				}
			}
			else
			{
				// F12
				if(scancode>0x1 && scancode<0xe)
				{
					if(scancode==0xd)
					{
						if(modifiers & (B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY | B_LEFT_OPTION_KEY))
						{
							// TODO: Set to Safe Mode here. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
					if(modifiers & (B_LEFT_SHIFT_KEY | B_LEFT_CONTROL_KEY))
					{
						STRACE(("Set Workspace %ld\n",scancode-1));
						//TODO: resolve			
						//SetWorkspace(scancode-2);
						break;
					}	
				}
				
				//Tab
				if(scancode==0x26 && (modifiers & B_SHIFT_KEY))
				{
					STRACE(("Twitcher\n"));
					ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}

				// Pause/Break
				if(scancode==0x7f)
				{
					if(GetDisplayDriver())
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							fScreenShotIndex++;
							sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						}
						fScreenShotIndex++;

						GetDisplayDriver()->DumpToFile(filename);
						break;
					}
				}
			}
			
			// We got this far, so apparently it's safe to pass to the active
			// window.

			// TODO: Pass on to client window with the focus
			break;
		}
		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) int8 number of bytes to follow containing the 
			//		generated string
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys
			
			// Obtain only what data which we'll need
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int8) * 3;
			int8 stringlength=*index; index+=stringlength + sizeof(int8);
			STRACE(("Key Up: 0x%lx\n",scancode));
			
			if(DISPLAYDRIVER==HWDRIVER)
			{
				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}
			}
			else
			{
				if(scancode==0x26 && (modifiers & B_LEFT_SHIFT_KEY))
				{
					ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.
			
			// TODO: Pass on to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			#ifdef DEBUG_KEYHANDLING
			index+=sizeof(bigtime_t);
			printf("Unmapped Key Down: 0x%lx\n",*((int32*)index));
			#endif
			// TODO: Pass on to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			#ifdef DEBUG_KEYHANDLING
			index+=sizeof(bigtime_t);
			printf("Unmapped Key Up: 0x%lx\n",*((int32*)index));
			#endif

			// TODO: Pass on to client window with the focus
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys
			
			#ifdef DEBUG_KEYHANDLING
			index+=sizeof(bigtime_t);
			printf("Modifiers Changed\n");
			#endif

			// TODO: Pass on to client window with the focus
			break;
		}
		default:
			break;
	}
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

//---------------------------------------------------------------------------
//				Methods for various desktop stuff handled by the server
//---------------------------------------------------------------------------
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

void Desktop::RemoveSubsetWindow(WinBorder* wb)
{
	WinBorder		*winBorder = NULL;

	fLayerLock.Lock();
	int32			count = fWinBorderList.CountItems();
	for(int32 i=0; i < count; i++)
	{
		winBorder	= static_cast<WinBorder*>(fWinBorderList.ItemAt(i));
		if (winBorder->fLevel == B_NORMAL_FEEL)
			winBorder->Window()->fWinFMWList.RemoveItem(wb);
	}
	fLayerLock.Unlock();

	RootLayer *rl = winBorder->GetRootLayer();

	if (!fGeneralLock.IsLocked())
		debugger("Desktop::RemoveWinBorder() - fGeneralLock must be locked!\n");
	if (!(rl->fMainLock.IsLocked()))
		debugger("Desktop::RemoveWinBorder() - fMainLock must be locked!\n");
		
	int32 countWKs = rl->WorkspaceCount();
	for (int32 i=0; i < countWKs; i++)
		rl->WorkspaceAt(i+1)->RemoveLayerPtr(wb);

}

void Desktop::PrintToStream(void)
{
	printf("RootLayer List:\n=======\n");
	for(int32 i=0; i<fRootLayerList.CountItems(); i++)
	{
		printf("\t%s\n", ((RootLayer*)fRootLayerList.ItemAt(i))->GetName());
		((RootLayer*)fRootLayerList.ItemAt(i))->PrintToStream();
		printf("-------\n");
	}
	printf("=======\nActive RootLayer: %s\n", fActiveRootLayer? fActiveRootLayer->GetName(): "NULL");
//	printf("Active WinBorder: %s\n", fActiveWinBorder? fActiveWinBorder->Name(): "NULL");
	
	printf("Screen List:\n");
	for(int32 i=0; i<fScreenList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
}

WinBorder* Desktop::FindWinBorderByServerWindowTokenAndTeamID(int32 token, team_id teamID)
{
	WinBorder*		wb;
	fLayerLock.Lock();
	for (int32 i = 0; (wb = (WinBorder*)fWinBorderList.ItemAt(i)); i++)
	{
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeamID() == teamID)
			break;
	}
	fLayerLock.Unlock();
	
	return wb;
}

void Desktop::PrintVisibleInRootLayerNo(int32 no)
{
	if (no<0 || no>=fRootLayerList.CountItems())
		return;

	printf("Visible windows in RootLayer %ld, Workspace %ld\n",
		ActiveRootLayerIndex(), ActiveRootLayer()->ActiveWorkspaceIndex());
	WinBorder *wb = NULL;
	Workspace *ws = ActiveRootLayer()->ActiveWorkspace();
	for(wb = (WinBorder*)ws->GoToTopItem(); wb != NULL; wb = (WinBorder*)ws->GoToLowerItem())
	{
		if (!wb->IsHidden())
			wb->PrintToStream();
	}
}
