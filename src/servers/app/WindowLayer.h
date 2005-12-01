/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@gmail.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef WINDOW_LAYER_H
#define WINDOW_LAYER_H


#include "Decorator.h"
#include "Layer.h"
#include "SubWindowList.h"

#include <Rect.h>
#include <String.h>


class ServerWindow;
class Decorator;
class DrawingEngine;
class Desktop;

class WindowLayer : public Layer {
 public:
								WindowLayer(const BRect &frame,
									const char *name, window_look look,
									window_feel feel, uint32 flags,
									uint32 workspaces, ServerWindow *window,
									DrawingEngine *driver);
	virtual						~WindowLayer();

	virtual	void				Draw(const BRect &r);

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	virtual	void				ScrollBy(float x, float y)
									{ /* not allowed */ }

	virtual	void				SetName(const char* name);

	virtual	bool				IsOffscreenWindow() const
									{ return false; }

	virtual	void				GetOnScreenRegion(BRegion& region);

			void				UpdateStart();
			void				UpdateEnd();
	inline	bool				InUpdate() const
									{ return fInUpdate; }
	inline	const BRegion&		RegionToBeUpdated() const
									{ return fInUpdateRegion; }
	inline	const BRegion&		CulmulatedUpdateRegion() const
									{ return fCumulativeRegion; }
			void				EnableUpdateRequests();
	inline	void				DisableUpdateRequests()
									{ fUpdateRequestsEnabled = false; }

			void				SetSizeLimits(float minWidth,
											  float maxWidth,
											  float minHeight,
											  float maxHeight);

			void				GetSizeLimits(float* minWidth,
											  float* maxWidth,
											  float* minHeight,
											  float* maxHeight) const;

	virtual	void				MouseDown(BMessage* message, BPoint where, int32* _viewToken);
	virtual	void				MouseUp(BMessage* message, BPoint where, int32* _viewToken);
	virtual	void				MouseMoved(BMessage* message, BPoint where, int32* _viewToken);

//			click_type			ActionFor(const BMessage *msg)
//									{ return _ActionFor(evt); }

	virtual	void				WorkspaceActivated(int32 index, bool active);
	virtual	void				WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces);
	virtual	void				Activated(bool active);

			void				UpdateColors();
			void				UpdateDecorator();
			void				UpdateFont();
			void				UpdateScreen();

			bool				IsFocus() const { return fIsFocus; }
			void				SetFocus(bool focus) { fIsFocus = focus; }

	inline	Decorator*			GetDecorator() const { return fDecorator; }

			window_look			Look() const { return fLook; }
			window_feel			Feel() const { return fFeel; }
			uint32				WindowFlags() const { return fWindowFlags; }

			uint32				Workspaces() const { return fWorkspaces; }
			void				SetWorkspaces(uint32 workspaces)
									{ fWorkspaces = workspaces; }
			bool				OnWorkspace(int32 index) const
									{ return (fWorkspaces & (1UL << index)) != 0; }

			bool				SupportsFront();

								// 0.0 -> left .... 1.0 -> right
			void				SetTabLocation(float location);
			float				TabLocation() const;

			void				HighlightDecorator(bool active);

			void				SetFeel(window_feel feel);
			void				SetLook(window_look look);

			SubWindowList		fSubWindowList;

			void				RequestClientRedraw(const BRegion& invalid);

			void				SetTopLayer(Layer* layer);
	inline	Layer*				TopLayer() const
									{ return fTopLayer; }

 protected:
	virtual void				_AllRedraw(const BRegion& invalid);

 private:
			void				set_decorator_region(BRect frame);
	virtual	void				_ReserveRegions(BRegion &reg);

	friend class RootLayer;

			click_type			_ActionFor(const BMessage *msg) const;

			Decorator*			fDecorator;
			Layer*				fTopLayer;

			BRegion				fCumulativeRegion;
			BRegion				fInUpdateRegion;

			BRegion				fDecRegion;
			bool				fRebuildDecRegion;

			int32				fMouseButtons;
			BPoint				fLastMousePosition;
			BPoint				fResizingClickOffset;

			bool				fIsFocus;

			bool				fIsClosing;
			bool				fIsMinimizing;
			bool				fIsZooming;
			bool				fIsResizing;
			bool				fIsSlidingTab;
			bool				fIsDragging;

			bool				fBringToFrontOnRelease;

			bool				fUpdateRequestsEnabled;
			bool				fInUpdate;
			bool				fRequestSent;

			window_look			fLook;
			window_feel			fFeel;
			uint32				fWindowFlags;
			uint32				fWorkspaces;

			float				fMinWidth;
			float				fMaxWidth;
			float				fMinHeight;
			float				fMaxHeight;
};

#endif	// WINDOW_LAYER_H
