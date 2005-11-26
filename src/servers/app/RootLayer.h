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
#ifndef ROOT_LAYER_H
#define ROOT_LAYER_H


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
class WindowLayer;

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

			void				HideWindowLayer(WindowLayer* windowLayer);
			void				ShowWindowLayer(WindowLayer* windowLayer);
			void				SetWindowLayerWorskpaces(WindowLayer *windowLayer,
									uint32 oldIndex, uint32 newIndex);

			void				RevealNewWMState(Workspace::State &oldWMState);
// TODO: we need to replace Winborder* with Layer*
	inline	WindowLayer*			Focus() const { return fWMState.Focus; }
	inline	WindowLayer*			Front() const { return fWMState.Front; }
	inline	WindowLayer*			Active() const { return fWMState.Focus; }
			bool				SetActive(WindowLayer* newActive, bool activate = true);

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

			void				SetDragMessage(BMessage *msg);
			BMessage*			DragMessage(void) const;

			void				SetMouseEventLayer(Layer* layer);

			void				LayerRemoved(Layer* layer);

			// Other methods
			bool				Lock() { return fAllRegionsLock.Lock(); }
			void				Unlock() { fAllRegionsLock.Unlock(); }
			bool				IsLocked() { return fAllRegionsLock.IsLocked(); }

			void				ChangeWindowLayerFeel(WindowLayer *windowLayer, int32 newFeel);

			void				MarkForRedraw(const BRegion &dirty);
			void				TriggerRedraw();

	virtual	void				Draw(const BRect &r);

			thread_id			LockingThread() { return fAllRegionsLock.LockingThread(); }

private:
friend class Desktop;

			// these are meant for Desktop class only!
			void				AddWindowLayer(WindowLayer* windowLayer);
			void				RemoveWindowLayer(WindowLayer* windowLayer);
			void				AddSubsetWindowLayer(WindowLayer *windowLayer, WindowLayer *toWindowLayer);
			void				RemoveSubsetWindowLayer(WindowLayer *windowLayer, WindowLayer *fromWindowLayer);

			void				MouseEventHandler(BMessage *msg);
			Layer*				_ChildAt(BPoint where);

			Desktop*			fDesktop;
			BMessage*			fDragMessage;
			Layer*				fMouseEventLayer;

			uint32				fSavedEventMask;
			uint32				fSavedEventOptions;

			BLocker				fAllRegionsLock;

			BRegion				fDirtyForRedraw;

			int32				fActiveWksIndex;
			int32				fWsCount;
			Workspace**			fWorkspace;
			Layer*				fWorkspacesLayer;

// TODO: fWMState MUST be associated with a surface. This is the case now
//   with RootLayer, but after Axel's refractoring this should go in
//   WorkspaceLayer, I think.
			Workspace::State	fWMState;

#if ON_SCREEN_DEBUGGING_INFO
	friend	class DebugInfoManager;
			void				AddDebugInfo(const char* string);
			BString				fDebugInfo;
#endif
#if DISPLAY_HAIKU_LOGO
			UtilityBitmap*		fLogoBitmap;
#endif
};

#endif	// ROOT_LAYER_H
