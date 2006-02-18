/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "WorkspacesLayer.h"

#include "AppServer.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "WindowLayer.h"
#include "Workspace.h"

#include <ColorSet.h>
#include <WindowPrivate.h>


WorkspacesLayer::WorkspacesLayer(BRect frame, const char* name,
	int32 token, uint32 resizeMode, uint32 flags)
	: ViewLayer(frame, name, token, resizeMode, flags)
{
	fDrawState->SetLowColor(RGBColor(255, 255, 255));
	fDrawState->SetHighColor(RGBColor(0, 0, 0));
}


WorkspacesLayer::~WorkspacesLayer()
{
}


void
WorkspacesLayer::_GetGrid(int32& columns, int32& rows)
{
	DesktopSettings settings(Window()->Desktop());
	int32 count = settings.WorkspacesCount();

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

	rect.OffsetBy(Frame().LeftTop());

	// make sure there is no gap anywhere
	if (column == columns - 1)
		rect.right = Frame().right;
	if (row == rows - 1)
		rect.bottom = Frame().bottom;

	return rect;
}


BRect
WorkspacesLayer::_WindowFrame(const BRect& workspaceFrame,
	const BRect& screenFrame, const BRect& windowFrame,
	BPoint windowPosition)
{
	BRect frame = windowFrame;
	frame.OffsetTo(windowPosition);

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
WorkspacesLayer::_DrawWindow(DrawingEngine* drawingEngine, const BRect& workspaceFrame,
	const BRect& screenFrame, WindowLayer* window, BPoint windowPosition,
	BRegion& backgroundRegion, bool active)
{
	if (window->Feel() == kDesktopWindowFeel || window->IsHidden())
		return;

	BPoint offset = window->Frame().LeftTop() - windowPosition;
	BRect frame = _WindowFrame(workspaceFrame, screenFrame, window->Frame(),
		windowPosition);
	Decorator *decorator = window->Decorator();
	BRect tabFrame(0, 0, 0, 0);
	if (decorator != NULL)
		tabFrame = decorator->TabRect();

	tabFrame = _WindowFrame(workspaceFrame, screenFrame,
		tabFrame, tabFrame.LeftTop() - offset);

	// ToDo: let decorator do this!
	RGBColor yellow;
	if (decorator != NULL)
		yellow = decorator->Colors().window_tab;
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

	if (decorator != NULL)
		drawingEngine->StrokeLine(tabFrame.LeftTop(), tabFrame.RightBottom(), yellow);

	drawingEngine->StrokeRect(frame, gray);

	frame.InsetBy(1, 1);
	drawingEngine->FillRect(frame, white);

	// draw title

	// TODO: disabled because it's much too slow this way - the mini-window
	//	functionality should probably be moved into the WindowLayer class,
	//	so that it has only to be recalculated on demand. With double buffered
	//	windows, this would also open up the door to have a more detailed
	//	preview.
#if 0
	BString title = window->Name();

	ServerFont font = fDrawState->Font();
	font.SetSize(7);
	fDrawState->SetFont(font);

	fDrawState->Font().TruncateString(&title, B_TRUNCATE_END, frame.Width() - 4);
	float width = GetDrawingEngine()->StringWidth(title.String(), title.Length(),
		fDrawState, NULL);
	float height = GetDrawingEngine()->StringHeight(title.String(), title.Length(),
		fDrawState);

	GetDrawingEngine()->DrawString(title.String(), title.Length(),
		BPoint(frame.left + (frame.Width() - width) / 2,
			frame.top + (frame.Height() + height) / 2),
		fDrawState, NULL);
#endif
}


void
WorkspacesLayer::_DrawWorkspace(DrawingEngine* drawingEngine,
	BRegion& redraw, int32 index)
{
	BRect rect = _WorkspaceAt(index);

	Workspace workspace(*Window()->Desktop(), index);
	bool active = workspace.IsCurrent();
	if (active) {
		// draw active frame
		RGBColor black(0, 0, 0);
		drawingEngine->StrokeRect(rect, black);
	}

	rect.InsetBy(1, 1);

	RGBColor color = workspace.Color();
	if (!active)
		_DarkenColor(color);

	// draw windows

	BRegion backgroundRegion = redraw;

	// ToDo: would be nice to get the real update region here

	uint16 width, height;
	uint32 colorSpace;
	float frequency;
	Window()->Desktop()->ScreenAt(0)->GetMode(width, height,
		colorSpace, frequency);
	BRect screenFrame(0, 0, width - 1, height - 1);

	BRegion workspaceRegion(rect);
	backgroundRegion.IntersectWith(&workspaceRegion);
	drawingEngine->ConstrainClippingRegion(&backgroundRegion);

	WindowLayer* window;
	BPoint leftTop;
	while (workspace.GetNextWindow(window, leftTop) == B_OK) {
		_DrawWindow(drawingEngine, rect, screenFrame, window,
			leftTop, backgroundRegion, active);
	}

	// draw background

	drawingEngine->ConstrainClippingRegion(&backgroundRegion);
	drawingEngine->FillRect(rect, color);

	drawingEngine->ConstrainClippingRegion(&redraw);
}


void
WorkspacesLayer::_DarkenColor(RGBColor& color) const
{
	color = tint_color(color.GetColor32(), B_DARKEN_2_TINT);
}


void
WorkspacesLayer::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping,
	BRegion* windowContentClipping, bool deep)
{
	// we can only draw within our own area
	BRegion redraw(ScreenClipping(windowContentClipping));
	// add the current clipping
	redraw.IntersectWith(effectiveClipping);

	int32 columns, rows;
	_GetGrid(columns, rows);

	// draw grid

	// make sure the grid around the active workspace is not drawn
	// to reduce flicker
	BRect activeRect = _WorkspaceAt(Window()->Desktop()->CurrentWorkspace());
	BRegion gridRegion(redraw);
	gridRegion.Exclude(activeRect);
	drawingEngine->ConstrainClippingRegion(&gridRegion);

	BRect frame = Frame();
		// top ViewLayer frame is in screen coordinates

	// horizontal lines

	drawingEngine->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.right, frame.top), ViewColor());

	for (int32 row = 0; row < rows; row++) {
		BRect rect = _WorkspaceAt(row * columns);
		drawingEngine->StrokeLine(BPoint(frame.left, rect.bottom),
			BPoint(frame.right, rect.bottom), ViewColor());
	}

	// vertical lines

	drawingEngine->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.left, frame.bottom), ViewColor());

	for (int32 column = 0; column < columns; column++) {
		BRect rect = _WorkspaceAt(column);
		drawingEngine->StrokeLine(BPoint(rect.right, frame.top),
			BPoint(rect.right, frame.bottom), ViewColor());
	}

	drawingEngine->ConstrainClippingRegion(&redraw);

	// draw workspaces

	for (int32 i = rows * columns; i-- > 0;) {
		_DrawWorkspace(drawingEngine, redraw, i);
	}
}


void
WorkspacesLayer::MouseDown(BMessage* message, BPoint where)
{
	int32 columns, rows;
	_GetGrid(columns, rows);

	for (int32 i = columns * rows; i-- > 0;) {
		BRect rect = _WorkspaceAt(i);

		if (rect.Contains(where)) {
			Window()->Desktop()->SetWorkspace(i);
			break;
		}
	}

	//ViewLayer::MouseDown(message, where, _viewToken);
}


void
WorkspacesLayer::WindowChanged(WindowLayer* window)
{
	// TODO: be smarter about this!
	BRegion region(Frame());
	Window()->MarkContentDirty(region);
}

