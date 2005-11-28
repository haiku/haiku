/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <ColorSet.h>
#include <WindowPrivate.h>

#include "AppServer.h"
#include "DrawingEngine.h"
#include "RootLayer.h"
#include "WindowLayer.h"
#include "Workspace.h"

#include "WorkspacesLayer.h"

WorkspacesLayer::WorkspacesLayer(BRect frame, const char* name,
	int32 token, uint32 resizeMode, uint32 flags, DrawingEngine* driver)
	: Layer(frame, name, token, resizeMode, flags, driver)
{
}


WorkspacesLayer::~WorkspacesLayer()
{
}


void
WorkspacesLayer::_GetGrid(int32& columns, int32& rows)
{
	int32 count = 4; //GetRootLayer()->WorkspaceCount();

	rows = 1;
	for (int32 i = 2; i < count; i++) {
		if (count % i == 0)
			rows = i;
	}

	columns = count / rows;
}


BRect
WorkspacesLayer::_WorkspaceAt(int32 i)
{
	int32 columns, rows;
	_GetGrid(columns, rows);

	int32 width = Frame().IntegerWidth() / columns;
	int32 height = Frame().IntegerHeight() / rows;

	int32 column = i % columns;
	int32 row = i / columns;

	BRect rect(column * width, row * height, (column + 1) * width, (row + 1) * height);

	// make sure there is no gap anywhere
	if (column == columns - 1)
		rect.right = Frame().right;
	if (row == rows - 1)
		rect.bottom = Frame().bottom;

	BPoint pt(0,0);
	ConvertToScreen(&pt);
	rect.OffsetBy(pt);
	return rect;
}


BRect
WorkspacesLayer::_WindowFrame(const BRect& workspaceFrame,
	const BRect& screenFrame, const BRect& windowFrame)
{
	BRect frame = windowFrame;

	float factor = workspaceFrame.Width() / screenFrame.Width();
	frame.left = rintf(frame.left * factor);
	frame.right = rintf(frame.right * factor);

	factor = workspaceFrame.Height() / screenFrame.Height();
	frame.top = rintf(frame.top * factor);
	frame.bottom = rintf(frame.bottom * factor);

	frame.OffsetBy(workspaceFrame.LeftTop());
	return frame;
}


void
WorkspacesLayer::_DrawWindow(const BRect& workspaceFrame,
	const BRect& screenFrame, WindowLayer* window,
	BRegion& backgroundRegion, bool active)
{
	if (window->Feel() == kDesktopWindowFeel)
		return;

	BRect frame = _WindowFrame(workspaceFrame, screenFrame, window->Frame());
	BRect tabFrame = _WindowFrame(workspaceFrame, screenFrame,
		window->GetDecorator()->GetTabRect());

	// ToDo: let decorator do this!
	RGBColor yellow = window->GetDecorator()->GetColors().window_tab;
	RGBColor gray(180, 180, 180);
	RGBColor white(255, 255, 255);

	if (!active) {
		_DarkenColor(yellow);
		_DarkenColor(gray);
		_DarkenColor(white);
	}

	if (tabFrame.left < frame.left)
		tabFrame.left = frame.left;
	if (tabFrame.right >= frame.right)
		tabFrame.right = frame.right - 1;

	tabFrame.top = frame.top - 1;
	tabFrame.bottom = frame.top - 1;

	backgroundRegion.Exclude(tabFrame);
	backgroundRegion.Exclude(frame);

	GetDrawingEngine()->StrokeLine(tabFrame.LeftTop(), tabFrame.RightBottom(), yellow);

	GetDrawingEngine()->StrokeRect(frame, gray);

	frame.InsetBy(1, 1);
	GetDrawingEngine()->FillRect(frame, white);
}


void
WorkspacesLayer::_DrawWorkspace(int32 index)
{
	BRect rect = _WorkspaceAt(index);

	Workspace* workspace = NULL;
	bool active = index == 0;
	if (active) {
		// draw active frame
		RGBColor black(0, 0, 0);
		GetDrawingEngine()->StrokeRect(rect, black);
	}

	// draw background

	rect.InsetBy(1, 1);
	RGBColor color;

	// ToDo: fix me - workspaces must always exist, not only on first visit!
	if (workspace != NULL)
		color = workspace->Color();
	else
		color.SetColor(51, 102, 152);

	if (!active) {
		_DarkenColor(color);
	}

	// draw windows

	BRegion backgroundRegion = VisibleRegion();

		// ToDo: would be nice to get the real update region here

	if (workspace != NULL) {
		WindowLayer* windows[256];
		int32 count = 0;
//		if (!workspace->GetWindowLayerList((void **)&windows, &count))
//			return;

		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		GetRootLayer()->GetDesktop()->ScreenAt(0)->GetMode(width, height, colorSpace, frequency);
		BRect screenFrame(0, 0, width - 1, height - 1);

		BRegion workspaceRegion(rect);
		backgroundRegion.IntersectWith(&workspaceRegion);
		GetDrawingEngine()->ConstrainClippingRegion(&backgroundRegion);

		for (int32 i = count; i-- > 0;) {
			_DrawWindow(rect, screenFrame, windows[i], backgroundRegion, active);
		}
	}

	GetDrawingEngine()->ConstrainClippingRegion(&backgroundRegion);
	GetDrawingEngine()->FillRect(rect, color);

	// TODO: ConstrainClippingRegion() should accept a const parameter !!
	BRegion cRegion(VisibleRegion());
	GetDrawingEngine()->ConstrainClippingRegion(&cRegion);
}


void
WorkspacesLayer::_DarkenColor(RGBColor& color) const
{
	color = tint_color(color.GetColor32(), B_DARKEN_2_TINT);
}


void
WorkspacesLayer::Draw(const BRect& updateRect)
{
	// ToDo: either draw into an off-screen bitmap, or turn off flickering...

	int32 columns, rows;
	_GetGrid(columns, rows);

	// draw grid
	// horizontal lines

	BRect frame = Frame();
	BPoint pt(0,0);
	ConvertToScreen(&pt);
	frame.OffsetBy(pt);

	GetDrawingEngine()->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.right, frame.top), ViewColor());

	for (int32 row = 0; row < rows; row++) {
		BRect rect = _WorkspaceAt(row * columns);
		GetDrawingEngine()->StrokeLine(BPoint(frame.left, rect.bottom),
			BPoint(frame.right, rect.bottom), ViewColor());
	}

	// vertical lines

	GetDrawingEngine()->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.left, frame.bottom), ViewColor());

	for (int32 column = 0; column < columns; column++) {
		BRect rect = _WorkspaceAt(column);
		GetDrawingEngine()->StrokeLine(BPoint(rect.right, frame.top),
			BPoint(rect.right, frame.bottom), ViewColor());
	}

	// draw workspaces

	int32 count = 4; //GetRootLayer()->WorkspaceCount();

	for (int32 i = 0; i < count; i++) {
		_DrawWorkspace(i);
	}
}

