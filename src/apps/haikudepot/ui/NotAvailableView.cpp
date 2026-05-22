/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "NotAvailableView.h"

#include <ControlLook.h>

#include "Logger.h"


const pattern _STIPPLE_PATTERN = {{0xc7, 0x8f, 0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3}};


/*!	These are placeholder graphics which show that an image would be here if
 *	there were one.
 */
class GeometricObjects {
public:
	/*! \param widthUnit is a length that defines the sizing of the geometric objects.
	 */
	GeometricObjects(float widthUnit) {
		// triangle
		float triangle_side_width = widthUnit * 1.3;
		float triangle_height = (triangle_side_width / 2) * tanf(M_PI / 3.0);
		float tBottom = triangle_height + widthUnit * 0.45;
		float tTop = tBottom - triangle_height;
		fTriangleBox = BRect(0, tTop, triangle_side_width, tBottom);

		// rectangle
		float rLeft = fTriangleBox.right + (widthUnit * 0.2);
		float rTop = fTriangleBox.bottom - (widthUnit * 0.3);
		fRectangleBox = BRect(rLeft, rTop, rLeft + widthUnit, rTop + widthUnit);

		// circle
		float cLeft = rLeft - (widthUnit * 0.5);
		float cTop = rTop - (widthUnit * 0.2 + widthUnit);
		fCircleBox = BRect(cLeft, cTop, cLeft + widthUnit, cTop + widthUnit);
	}

	/*!	\param view is a pointer to the BView instance to draw into.
	 *	\param pt is the point at which to draw the geometric objects.
	 */
	void DrawAt(BView* view, BPoint pt) {
		BRect rectangleBoxAtPt(fRectangleBox);
		BRect circleBoxAtPt(fCircleBox);
		BRect triangleAtPt(fTriangleBox);

		rectangleBoxAtPt.OffsetBy(pt);
		circleBoxAtPt.OffsetBy(pt);
		triangleAtPt.OffsetBy(pt);

		view->FillRect(rectangleBoxAtPt);
		view->FillEllipse(circleBoxAtPt);
		BPoint t[3] = {triangleAtPt.LeftBottom(),triangleAtPt.RightBottom(),
			BPoint(triangleAtPt.left + triangleAtPt.Width() / 2.0, triangleAtPt.top)};
		view->FillPolygon(t, 3);
	}

	/*!	\return the size of the geometric objects when rendered.
	 */
	BSize Size() {
		return (fTriangleBox | fCircleBox | fRectangleBox).Size();
	}

private:
	BRect fTriangleBox;
	BRect fCircleBox;
	BRect fRectangleBox;
};


NotAvailableView::NotAvailableView(const char* name, const char* text, bool isImage)
	:
	BView(name, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW),
	fIsImage(isImage),
	fText(text)
{
}


NotAvailableView::~NotAvailableView()
{
}


BSize
NotAvailableView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
		// can be any size
}


BAlignment
NotAvailableView::LayoutAlignment()
{
	return BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE);
}


void
NotAvailableView::SetText(const char* text)
{
	if (fText != text) {
		fText = text;
		Invalidate();
	}
}


const char*
NotAvailableView::Text() const
{
	return fText;
}


void
NotAvailableView::Draw(BRect updateRect)
{
	if (fIsImage)
		_DrawIsImage(updateRect);
	else
		_DrawText(Bounds(), updateRect);
}


/*!	In the case that the "not available" thing is an image then display a
 *	placeholder in the form of simple geometric objects and a message. The
 *	display should adjust depending on the space available.
 */
void
NotAvailableView::_DrawIsImage(BRect updateRect)
{
	BRect bounds = Bounds();

	// Use the width of three "M" characters as a unit of width to decide the
	// size of the geometric objects to render. If the user is using a high-res
	// monitor then this will scale accordingly.
	float std_width = StringWidth("M") * 3;

	float padding = be_control_look->DefaultItemSpacing();

	// Get the sizes of the objects that need to be rendered.
	GeometricObjects geometricObjects(std_width);
	BSize geometricsObjectsSize = geometricObjects.Size();
	float minHeightRequiredForGeometricObjects = geometricsObjectsSize.Height() + padding * 2.0;

	_DrawStippledBorder(bounds);

	if (bounds.Height() / 2.0 < minHeightRequiredForGeometricObjects) {
		_DrawText(bounds, updateRect);
	} else {
		float middle = bounds.top + (bounds.Height() / 2.0);

		PushState();

		SetHighUIColor(ViewUIColor(), B_DARKEN_1_TINT);

		// Draw the geometric objects the represent the missing image just above
		// the centerline of the region to draw in. The text gos under this area.

		BPoint geometricObjectsDrawPt((bounds.left + bounds.Width() / 2)
				- geometricsObjectsSize.width / 2.0,
			middle - geometricsObjectsSize.height);
		geometricObjects.DrawAt(this, geometricObjectsDrawPt);

		PopState();

		BRect textBox = BRect(bounds.left, middle, bounds.right, bounds.bottom);
		textBox.InsetBy(padding, padding);
		_DrawText(textBox, updateRect);
	}
}


void
NotAvailableView::_DrawStippledBorder(BRect box)
{
	PushState();
	SetHighUIColor(ViewUIColor(), B_DARKEN_1_TINT);
	StrokeRect(box, _STIPPLE_PATTERN);
	PopState();
}


void
NotAvailableView::_DrawText(BRect textRect, BRect updateRect)
{
	be_control_look->DrawLabel(this, fText, textRect, updateRect, ViewColor(),
		BControlLook::B_DISABLED, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
}
