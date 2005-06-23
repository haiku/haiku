//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved.
//  Distributed under the terms of the MIT license.
//
//	File Name:		Desktop.h
//	Author:			Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------
#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <InterfaceDefs.h>
#include <List.h>
#include <Locker.h>
#include <Menu.h>

class BMessage;
class BPortLink;
class DisplayDriver;
class HWInterface;
class Layer;
class RootLayer;
class Screen;
class WinBorder;

class Desktop {
 public:
	// startup methods
								Desktop();
	virtual						~Desktop();

			void				Init();

	// 1-BigScreen or n-SmallScreens
			void				InitMode();
	
	// Methods for multiple monitors.
			Screen*				ScreenAt(int32 index) const;
			int32				ScreenCount() const;
			Screen*				ActiveScreen() const;
	
			void				SetActiveRootLayerByIndex(int32 index);
			void				SetActiveRootLayer(RootLayer* layer);
			RootLayer*			RootLayerAt(int32 index);
			RootLayer*			ActiveRootLayer() const;
			int32				ActiveRootLayerIndex() const;
			int32				CountRootLayers() const;

			DisplayDriver*		GetDisplayDriver() const;
			HWInterface*		GetHWInterface() const;
	
	// Methods for layer(WinBorder) manipulation.
			void				AddWinBorder(WinBorder *winBorder);
			void				RemoveWinBorder(WinBorder *winBorder);
			void				SetWinBorderFeel(WinBorder *winBorder,
												 uint32 feel);
			void				AddWinBorderToSubset(WinBorder *winBorder,
													 WinBorder *toWinBorder);
			void				RemoveWinBorderFromSubset(WinBorder *winBorder,
														  WinBorder *fromWinBorder);

			WinBorder*			FindWinBorderByServerWindowTokenAndTeamID(int32 token,
																		  team_id teamID);
	// get list of registed windows
			const BList&		WindowList() const
								{
									if (!IsLocked())
										debugger("You must lock before getting registered windows list\n");
									return fWinBorderList;
								}

	// locking with regards to registered windows list
			bool				Lock()
									{ return fWinLock.Lock(); }
			void				Unlock()
									{ return fWinLock.Unlock(); }
			bool				IsLocked() const
									{ return fWinLock.IsLocked(); }

	// Methods for various desktop stuff handled by the server
			void				SetScrollBarInfo(const scroll_bar_info &info);
			scroll_bar_info		ScrollBarInfo() const;
	
			void				SetMenuInfo(const menu_info &info);
			menu_info			MenuInfo() const;
	
			void				UseFFMouse(const bool &useffm);
			bool				FFMouseInUse() const;
			void				SetFFMouseMode(const mode_mouse &value);
			mode_mouse			FFMouseMode() const;
	
	// Debugging methods
			void				PrintToStream();
			void				PrintVisibleInRootLayerNo(int32 no);
	
 private:
			void				_AddGraphicsCard(HWInterface* interface);

			BList				fWinBorderList;
			BLocker				fWinLock;
		
			BList				fRootLayerList;
			RootLayer*			fActiveRootLayer;
		
			BList				fScreenList;
			Screen*				fActiveScreen;
			
			scroll_bar_info		fScrollBarInfo;
			menu_info			fMenuInfo;
			mode_mouse			fMouseMode;
			bool				fFFMouseMode;
};

extern Desktop *gDesktop;

#endif // _DESKTOP_H_
