/*
 * Copyright 2005-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACES_VIEW_H
#define WORKSPACES_VIEW_H


#include "View.h"


class WorkspacesView : public View {
public:
					WorkspacesView(BRect frame, BPoint scrollingOffset,
						const char* name, int32 token, uint32 resize,
						uint32 flags);
	virtual			~WorkspacesView();

	virtual	void	AttachedToWindow(::Window* window);
	virtual	void	DetachedFromWindow();

	virtual	void	Draw(DrawingEngine* drawingEngine,
						BRegion* effectiveClipping,
						BRegion* windowContentClipping, bool deep = false);

	virtual	void	MouseDown(BMessage* message, BPoint where);
	virtual	void	MouseUp(BMessage* message, BPoint where);
	virtual	void	MouseMoved(BMessage* message, BPoint where);

			void	WindowChanged(::Window* window);
			void	WindowRemoved(::Window* window);

private:
			void	_GetGrid(int32& columns, int32& rows);
			BRect	_ScreenFrame(int32 index);
			BRect	_WorkspaceAt(int32 index);
			BRect	_WorkspaceAt(BPoint where, int32& index);
			BRect	_WindowFrame(const BRect& workspaceFrame,
						const BRect& screenFrame, const BRect& windowFrame,
						BPoint windowPosition);

			void	_DrawWindow(DrawingEngine* drawingEngine,
						const BRect& workspaceFrame, const BRect& screenFrame,
						::Window* window, BPoint windowPosition,
						BRegion& backgroundRegion, bool workspaceActive);
			void	_DrawWorkspace(DrawingEngine* drawingEngine,
						BRegion& redraw, int32 index);

			void	_DarkenColor(rgb_color& color) const;
			void	_Invalidate() const;

private:
	::Window*		fSelectedWindow;
	int32			fSelectedWorkspace;
	bool			fHasMoved;
	BPoint			fClickPoint;
	BPoint			fLeftTopOffset;
};

#endif	// WORKSPACES_VIEW_H
