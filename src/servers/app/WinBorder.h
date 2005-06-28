/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@cotty.iren.ro>
 *			Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef _WINBORDER_H_
#define _WINBORDER_H_

#include <Rect.h>
#include <String.h>
#include "Layer.h"
#include "SubWindowList.h"
#include "Decorator.h"

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
class DisplayDriver;
class Desktop;

class PointerEvent {
 public:
	int32		code;		// B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED,
							// B_MOUSE_WHEEL_CHANGED
	bigtime_t	when;
	BPoint		where;
	float		wheel_delta_x;
	float		wheel_delta_y;
	int32		modifiers;
	int32		buttons;	// B_PRIMARY_MOUSE_BUTTON, B_SECONDARY_MOUSE_BUTTON
							// B_TERTIARY_MOUSE_BUTTON
	int32		clicks;
};

class WinBorder : public Layer {
 public:
								WinBorder(const BRect &frame,
										  const char *name,
										  const uint32 look,
										  const uint32 feel, 
										  const uint32 flags,
										  const uint32 workspaces,
										  ServerWindow *window,
										  DisplayDriver *driver);
	virtual						~WinBorder();
	
	virtual	void				Draw(const BRect &r);
	
	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
#ifndef NEW_CLIPPING
	virtual	void				RebuildFullRegion();
#endif

			void				UpdateStart();
			void				UpdateEnd();
	inline	bool				InUpdate() const
									{ return fInUpdate; }
	inline	const BRegion&		RegionToBeUpdated() const
									{ return fInUpdateRegion; }

			void				SetSizeLimits(float minWidth,
											  float maxWidth,
											  float minHeight,
											  float maxHeight);

			void				GetSizeLimits(float* minWidth,
											  float* maxWidth,
											  float* minHeight,
											  float* maxHeight) const;

			click_type			MouseDown(const PointerEvent& evt);
			void				MouseMoved(const PointerEvent& evt);
			void				MouseUp(const PointerEvent& evt);
	
			void				UpdateColors();
			void				UpdateDecorator();
			void				UpdateFont();
			void				UpdateScreen();

	virtual bool				HasClient() { return false; }
	inline	Decorator*			GetDecorator() const { return fDecorator; }

	inline	int32				Look() const { return fLook; }
	inline	int32				Feel() const { return fFeel; }
	inline	int32				Level() const { return fLevel; }
	inline	uint32				WindowFlags() const { return fWindowFlags; }
	inline	uint32				Workspaces() const { return fWorkspaces; }

								// 0.0 -> left .... 1.0 -> right
			void				SetTabLocation(float location);
			float				TabLocation() const;

			void				HighlightDecorator(bool active);
	
			bool				HasPoint(const BPoint &pt) const;

	inline	void				QuietlySetWorkspaces(uint32 wks) { fWorkspaces = wks; }	
			void				QuietlySetFeel(int32 feel);	

			SubWindowList		fSubWindowList;

#ifdef NEW_CLIPPING
 public:
	virtual	void				MovedByHook(float dx, float dy);
	virtual	void				ResizedByHook(float dx, float dy, bool automatic);

 private:
			void				set_decorator_region(BRect frame);
	virtual	bool				alter_visible_for_children(BRegion &region);
	virtual	void				get_user_regions(BRegion &reg);

			BRegion				fDecRegion;
			bool				fRebuildDecRegion;

#endif

 protected:
	friend class Layer;
	friend class ServerWindow;
	friend class RootLayer;

			click_type			_ActionFor(const PointerEvent& evt) const;
			bool				_ResizeBy(float x, float y);

			Decorator*			fDecorator;
			Layer*				fTopLayer;

			BRegion				fCumulativeRegion;
			BRegion				fInUpdateRegion;

			int32				fMouseButtons;
			int32				fKeyModifiers;
			BPoint				fLastMousePosition;
			BPoint				fResizingClickOffset;

			bool				fIsClosing;
			bool				fIsMinimizing;
			bool				fIsZooming;

			bool				fIsDragging;
			bool				fBringToFrontOnRelease;

			bool				fIsResizing;

			bool				fIsSlidingTab;

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

			int cnt; // for debugging
};

#endif
