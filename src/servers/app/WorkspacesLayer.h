/*
 * Copyright 2005-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACES_LAYER_H
#define WORKSPACES_LAYER_H


#include "ViewLayer.h"

class WindowLayer;


class WorkspacesLayer : public ViewLayer {
	public:
		WorkspacesLayer(BRect frame, BPoint scrollingOffset, const char* name,
			int32 token, uint32 resize, uint32 flags);
		virtual ~WorkspacesLayer();

		virtual	void Draw(DrawingEngine* drawingEngine,
						BRegion* effectiveClipping,
						BRegion* windowContentClipping,
						bool deep = false);

		virtual void MouseDown(BMessage* message, BPoint where);
		virtual void MouseUp(BMessage* message, BPoint where);
		virtual void MouseMoved(BMessage* message, BPoint where);

		void WindowChanged(WindowLayer* window);
		void WindowRemoved(WindowLayer* window);

	private:
		void _GetGrid(int32& columns, int32& rows);
		BRect _ScreenFrame(int32 index);
		BRect _WorkspaceAt(int32 index);
		BRect _WorkspaceAt(BPoint where, int32& index);
		BRect _WindowFrame(const BRect& workspaceFrame,
					const BRect& screenFrame, const BRect& windowFrame,
					BPoint windowPosition);

		void _DrawWindow(DrawingEngine* drawingEngine, const BRect& workspaceFrame,
					const BRect& screenFrame, WindowLayer* window,
					BPoint windowPosition, BRegion& backgroundRegion,
					bool active);
		void _DrawWorkspace(DrawingEngine* drawingEngine, BRegion& redraw,
					int32 index);

		void _DarkenColor(RGBColor& color) const;
		void _Invalidate() const;

	private:
		WindowLayer*	fSelectedWindow;
		int32			fSelectedWorkspace;
		bool			fHasMoved;
		BPoint			fLeftTopOffset;
};

#endif	// WORKSPACES_LAYER_H
