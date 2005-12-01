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


class RootLayer : public Layer {
	public:
							RootLayer(const char *name,	Desktop *desktop,
								DrawingEngine *driver);
		virtual				~RootLayer();

		Desktop*			GetDesktop() const { return fDesktop; }

		virtual	void		MoveBy(float x, float y);
		virtual	void		ResizeBy(float x, float y);
		virtual	void		ScrollBy(float x, float y)
								{ /* not allowed */ }

		void				HideWindowLayer(WindowLayer* windowLayer);
		void				ShowWindowLayer(WindowLayer* windowLayer, bool toFront = true);

		void				MoveWindowBy(WindowLayer* window, float x, float y);
		void				ResizeWindowBy(WindowLayer* window, float x, float y);

		bool				SetFocus(WindowLayer* focus);
		WindowLayer*		Focus() const { return fFocus; }
		WindowLayer*		Front() const { return fFront; }
		WindowLayer*		Back() const { return fBack; }

		void				SetWorkspace(int32 index,
								Workspace::Private& previousWorkspace,
								Workspace::Private& workspace);

		void				SetDragMessage(BMessage *msg);
		BMessage*			DragMessage() const;

		WindowLayer*		WindowAt(BPoint where);

		WindowLayer*		MouseEventWindow() const { return fMouseEventWindow; }
		void				SetMouseEventWindow(WindowLayer* layer);

		void				LayerRemoved(Layer* layer);

		// Other methods
		bool				Lock() { return fAllRegionsLock.Lock(); }
		void				Unlock() { fAllRegionsLock.Unlock(); }
		bool				IsLocked() { return fAllRegionsLock.IsLocked(); }

		void				ActivateWindow(WindowLayer* window);
		void				SendBehindWindow(WindowLayer* window, WindowLayer* front);

		void				SetWindowLayerFeel(WindowLayer *windowLayer, int32 newFeel);
		void				SetWindowLayerLook(WindowLayer *windowLayer, int32 newLook);

		void				UpdateWorkspaces();

		void				MarkForRedraw(const BRegion &dirty);
		void				TriggerRedraw();

		void				Draw(const BRect &r);

		thread_id			LockingThread() { return fAllRegionsLock.LockingThread(); }

		void				AddWindowLayer(WindowLayer* windowLayer);
		void				RemoveWindowLayer(WindowLayer* windowLayer);

	private:
		bool				_SetFocus(WindowLayer* focus, BRegion& update);
		void				_SetFront(WindowLayer* front, BRegion& update);
		void				_UpdateBack();
		void				_UpdateFront();
		void				_UpdateFronts();

		void				_WindowsChanged(BRegion& region);
		void				_UpdateWorkspace(Workspace::Private& workspace);

		Desktop*			fDesktop;
		BMessage*			fDragMessage;
		WindowLayer*		fMouseEventWindow;

		BLocker				fAllRegionsLock;

		BRegion				fDirtyForRedraw;

		int32				fWorkspace;
		RGBColor			fColor;
		Layer*				fWorkspacesLayer;

		WindowLayer*		fFocus;
		WindowLayer*		fFront;
		WindowLayer*		fBack;

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
