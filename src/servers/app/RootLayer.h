/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used for the top layer of each workspace's Layer tree */

#ifndef _ROOTLAYER_H_
#define _ROOTLAYER_H_

#include <List.h>
#include <Locker.h>
#include <MessageQueue.h>

#include "DebugInfoManager.h"
#include "Desktop.h"
#include "Layer.h"
#include "Workspace.h"

class DrawingEngine;
class HWInterface;
class RGBColor;
class Screen;
class WinBorder;

namespace BPrivate {
	class PortLink;
};

#ifndef DISPLAY_HAIKU_LOGO
#	define DISPLAY_HAIKU_LOGO 1
#endif

#if DISPLAY_HAIKU_LOGO
class UtilityBitmap;
#endif

/*!
	\class RootLayer RootLayer.h
	\brief Class used for the top layer of each workspace's Layer tree
	
	RootLayers are used to head up the top of each Layer tree and reimplement certain 
	Layer functions to act accordingly. There is only one for each workspace class.
	
*/
class RootLayer : public Layer {
public:
								RootLayer(const char *name,	int32 workspaceCount,
									Desktop *desktop, DrawingEngine *driver);
	virtual						~RootLayer(void);

	Desktop*					GetDesktop() const { return fDesktop; }

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	virtual	void				ScrollBy(float x, float y)
									{ /* not allowed */ }

	// For the active workspaces
	virtual	Layer*				FirstChild() const;
	virtual	Layer*				NextChild() const;
	virtual	Layer*				PreviousChild() const;
	virtual	Layer*				LastChild() const;

			void				HideWinBorder(WinBorder* winBorder);
			void				ShowWinBorder(WinBorder* winBorder);
			void				SetWinBorderWorskpaces(WinBorder *winBorder,
									uint32 oldIndex, uint32 newIndex);

			void				RevealNewWMState(Workspace::State &oldWMState);
// TODO: we need to replace Winborder* with Layer*
	inline	WinBorder*			Focus() const { return fWMState.Focus; }
	inline	WinBorder*			Front() const { return fWMState.Front; }
	inline	WinBorder*			Active() const { return fWMState.Active; }
			bool				SetActive(WinBorder* newActive, bool activate = true);

	inline	void				SetWorkspaceCount(int32 wksCount);
	inline	int32				WorkspaceCount() const { return fWsCount; }
	inline	Workspace*			WorkspaceAt(int32 index) const { return fWorkspace[index]; }
	inline	Workspace*			ActiveWorkspace() const { return fWorkspace[fActiveWksIndex]; }
	inline	int32				ActiveWorkspaceIndex() const { return fActiveWksIndex; }
			bool				SetActiveWorkspace(int32 index);

			void				ReadWorkspaceData(const char *path);
			void				SaveWorkspaceData(const char *path);

			void				SetWorkspacesLayer(Layer* layer) { fWorkspacesLayer = layer; }
			Layer*				WorkspacesLayer() const { return fWorkspacesLayer; }
	
			void				SetBGColor(const RGBColor &col);
			RGBColor			BGColor(void) const;
	
	inline	int32				Buttons(void) { return fButtons; }
	
			void				SetDragMessage(BMessage *msg);
			BMessage*			DragMessage(void) const;

			bool				AddToInputNotificationLists(Layer *lay, uint32 mask,
									uint32 options);
			bool				SetNotifyLayer(Layer *lay, uint32 mask, uint32 options);
			void				ClearNotifyLayer();

			void				LayerRemoved(Layer* layer);

	static	int32				WorkingThread(void *data);

			// Other methods
			bool				Lock() { return fAllRegionsLock.Lock(); }
			void				Unlock() { fAllRegionsLock.Unlock(); }
			bool				IsLocked() { return fAllRegionsLock.IsLocked(); }
			void				RunThread();

			void				GoChangeWinBorderFeel(WinBorder *winBorder, int32 newFeel);

			void				MarkForRedraw(const BRegion &dirty);
			void				TriggerRedraw();

	virtual	void				Draw(const BRect &r);

			thread_id			LockingThread() { return fAllRegionsLock.LockingThread(); }

//			BRegion				fRedrawReg;
//			BList				fCopyRegList;
//			BList				fCopyList;

private:
friend class Desktop;

			// these are meant for Desktop class only!
			void				AddWinBorder(WinBorder* winBorder);
			void				RemoveWinBorder(WinBorder* winBorder);
			void				AddSubsetWinBorder(WinBorder *winBorder, WinBorder *toWinBorder);
			void				RemoveSubsetWinBorder(WinBorder *winBorder, WinBorder *fromWinBorder);

			void				show_winBorder(WinBorder* winBorder);
			void				hide_winBorder(WinBorder* winBorder);

			void				change_winBorder_feel(WinBorder *winBorder, int32 newFeel);

			// Input related methods
			void				MouseEventHandler(BMessage *msg);
			void				KeyboardEventHandler(BMessage *msg);

			void				_ProcessMouseMovedEvent(BMessage *msg);

	inline	HWInterface*		GetHWInterface() const
									{ return fDesktop->GetHWInterface(); }

			void*				ReadRawFromPort(int32 *msgCode, bigtime_t timeout);
			BMessage*			ReadMessageFromPort(bigtime_t tout);
			BMessage*			ConvertToMessage(void* raw, int32 code);

			Desktop*			fDesktop;
			BMessage*			fDragMessage;
			Layer*				fLastLayerUnderMouse;

			Layer*				fNotifyLayer;
			uint32				fSavedEventMask;
			uint32				fSavedEventOptions;
			BList				fMouseNotificationList;
			BList				fKeyboardNotificationList;

			BLocker				fAllRegionsLock;

			BRegion				fDirtyForRedraw;

			thread_id			fThreadID;
			port_id				fListenPort;
			BMessageQueue		fQueue;

			int32				fButtons;
			BPoint				fLastMousePosition;
	
			int32				fActiveWksIndex;
			int32				fWsCount;
			Workspace**			fWorkspace;
			Layer*				fWorkspacesLayer;

// TODO: fWMState MUST be associated with a surface. This is the case now
//   with RootLayer, but after Axel's refractoring this should go in
//   WorkspaceLayer, I think.
			Workspace::State	fWMState;
	mutable int32				fWinBorderIndex;

			int32				fScreenShotIndex;
			bool				fQuiting;

#if ON_SCREEN_DEBUGGING_INFO
	friend	class DebugInfoManager;
			void				AddDebugInfo(const char* string);
			BString				fDebugInfo;
#endif
#if DISPLAY_HAIKU_LOGO
			UtilityBitmap*		fLogoBitmap;
#endif
};

#endif
