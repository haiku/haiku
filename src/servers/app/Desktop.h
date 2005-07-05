/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _DESKTOP_H_
#define _DESKTOP_H_


#include <InterfaceDefs.h>
#include <List.h>
#include <Locker.h>
#include <Menu.h>
#include <Autolock.h>

#include "ServerScreen.h"

class BMessage;

class DisplayDriver;
class HWInterface;
class Layer;
class RootLayer;
class WinBorder;

namespace BPrivate {
	class LinkSender;
};


class Desktop : public BLocker {
 public:
	// startup methods
								Desktop();
	virtual						~Desktop();

			void				Init();

	// 1-BigScreen or n-SmallScreens
			void				InitMode();
	
	// Methods for multiple monitors.
	inline	Screen*				ScreenAt(int32 index) const
									{ return static_cast<Screen *>(fScreenList.ItemAt(index)); }
	inline	int32				ScreenCount() const
									{ return fScreenList.CountItems(); }
	inline	Screen*				ActiveScreen() const
									{ return fActiveScreen; }

			void				SetActiveRootLayerByIndex(int32 index);
			void				SetActiveRootLayer(RootLayer* layer);
	inline	RootLayer*			RootLayerAt(int32 index)
									{ return static_cast<RootLayer *>(fRootLayerList.ItemAt(index)); }
			RootLayer*			ActiveRootLayer() const
									{ return fActiveRootLayer;}
			int32				ActiveRootLayerIndex() const
									{
										int32 rootLayerCount = CountRootLayers();

										for (int32 i = 0; i < rootLayerCount; i++) {
											if (fActiveRootLayer == (RootLayer *)fRootLayerList.ItemAt(i))
											return i;
										}
										return -1;
									}

	inline	int32				CountRootLayers() const
									{ return fRootLayerList.CountItems(); }

	inline	DisplayDriver*		GetDisplayDriver() const
									{ return ScreenAt(0)->GetDisplayDriver(); }
	inline	HWInterface*		GetHWInterface() const
									{ return ScreenAt(0)->GetHWInterface(); }

	// Methods for layer(WinBorder) manipulation.
			void				AddWinBorder(WinBorder *winBorder);
			void				RemoveWinBorder(WinBorder *winBorder);
			void				SetWinBorderFeel(WinBorder *winBorder,
												 uint32 feel);
			void				AddWinBorderToSubset(WinBorder *winBorder,
													 WinBorder *toWinBorder);
			void				RemoveWinBorderFromSubset(WinBorder *winBorder,
														  WinBorder *fromWinBorder);

			WinBorder*			FindWinBorderByClientToken(int32 token, team_id teamID);
			//WinBorder*		FindWinBorderByServerToken(int32 token);

	// get list of registed windows
			const BList&		WindowList() const
								{
									if (!IsLocked())
										debugger("You must lock before getting registered windows list\n");
									return fWinBorderList;
								}

			void				WriteWindowList(team_id team, BPrivate::LinkSender& sender);
			void				WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender);

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

#endif	// _DESKTOP_H_
