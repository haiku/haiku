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


// these are used by window manager to properly place window.
enum {
	B_SYSTEM_LAST	= -10L,

	B_FLOATING_APP	= 0L,
	B_MODAL_APP		= 1L,
	B_NORMAL		= 2L,
	B_FLOATING_ALL	= 3L,
	B_MODAL_ALL		= 4L,

	B_SYSTEM_FIRST	= 10L,
};

class ServerWindow;
class Decorator;
class DrawingEngine;
class Desktop;

class WindowLayer : public Layer {
 public:
								WindowLayer(const BRect &frame,
										  const char *name,
										  const uint32 look,
										  const uint32 feel, 
										  const uint32 flags,
										  const uint32 workspaces,
										  ServerWindow *window,
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

	virtual	void				MouseDown(BMessage *msg, BPoint where, int32* _viewToken);
	virtual	void				MouseUp(BMessage *msg, BPoint where, int32* _viewToken);
	virtual	void				MouseMoved(BMessage *msg, BPoint where, int32* _viewToken);

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

	inline	int32				Look() const { return fLook; }
	inline	int32				Feel() const { return fFeel; }
	inline	int32				Level() const { return fLevel; }
	inline	uint32				WindowFlags() const { return fWindowFlags; }
	inline	uint32				Workspaces() const { return fWorkspaces; }
			void				SetWorkspaces(uint32 workspaces)
									{ fWorkspaces = workspaces; }

			bool				SupportsFront();

								// 0.0 -> left .... 1.0 -> right
			void				SetTabLocation(float location);
			float				TabLocation() const;

			void				HighlightDecorator(bool active);

	inline	void				QuietlySetWorkspaces(uint32 wks) { fWorkspaces = wks; }	
			void				QuietlySetFeel(int32 feel);	

			SubWindowList		fSubWindowList;

			void				RequestClientRedraw(const BRegion& invalid);
	virtual void				_AllRedraw(const BRegion& invalid);

			void				SetTopLayer(Layer* layer);
	inline	Layer*				TopLayer() const
									{ return fTopLayer; }

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

			int32				fLook;
			int32				fFeel;
			int32				fLevel;
			int32				fWindowFlags;
			uint32				fWorkspaces;

			float				fMinWidth;
			float				fMaxWidth;
			float				fMinHeight;
			float				fMaxHeight;
};

#endif	// WINDOW_LAYER_H
