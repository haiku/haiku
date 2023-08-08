/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */


#include "PerspectiveTransformer.h"

#include <new>
#include <stdio.h>

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <Message.h>

#include "Shape.h"


_USING_ICON_NAMESPACE
using std::nothrow;


PerspectiveTransformer::PerspectiveTransformer(VertexSource& source, Shape* shape)
	: Transformer("Perspective"),
	  PathTransformer(source),
	  Perspective(source, *this),
	  fShape(shape)
#ifdef ICON_O_MATIC
	, fInverted(false)
#endif
{
#ifdef ICON_O_MATIC
	if (fShape != NULL) {
		fShape->AcquireReference();
		fShape->AddObserver(this);
		ObjectChanged(fShape); // finish initialization
	}
#endif
}


PerspectiveTransformer::PerspectiveTransformer(
		VertexSource& source, Shape* shape, BMessage* archive)
	: Transformer(archive),
	  PathTransformer(source),
	  Perspective(source, *this),
	  fShape(shape)
#ifdef ICON_O_MATIC
	, fInverted(false)
#endif
{
	double matrix[9];
	for (int i = 0; i < 9; i++) {
		if (archive->FindDouble("matrix", i, &matrix[i]) != B_OK)
			matrix[i] = 0;
	}
	load_from(matrix);

#ifdef ICON_O_MATIC
	if (fShape != NULL) {
		fShape->AcquireReference();
		fShape->AddObserver(this);
		ObjectChanged(fShape); // finish initialization
	}
#endif
}


PerspectiveTransformer::PerspectiveTransformer(const PerspectiveTransformer& other)
#ifdef ICON_O_MATIC
	: Transformer(other.Name()),
#else
	: Transformer(""),
#endif
	  PathTransformer(other.fSource),
	  Perspective(fSource, *this),
	  fShape(other.fShape)
#ifdef ICON_O_MATIC
	, fInverted(other.fInverted),
	  fFromBox(other.fFromBox),
	  fToLeftTop(other.fToLeftTop),
	  fToRightTop(other.fToRightTop),
	  fToLeftBottom(other.fToLeftBottom),
	  fToRightBottom(other.fToRightBottom),
	  fValid(other.fValid)
#endif
{
	double matrix[9];
	other.store_to(matrix);
	load_from(matrix);

#ifdef ICON_O_MATIC
	if (fShape != NULL) {
		fShape->AcquireReference();
		fShape->AddObserver(this);
	}
#endif
}


PerspectiveTransformer::~PerspectiveTransformer()
{
#ifdef ICON_O_MATIC
	if (fShape != NULL) {
		fShape->RemoveObserver(this);
		fShape->ReleaseReference();
	}
#endif
}


// #pragma mark -


Transformer*
PerspectiveTransformer::Clone() const
{
	return new (nothrow) PerspectiveTransformer(*this);
}


// #pragma mark -


void
PerspectiveTransformer::rewind(unsigned path_id)
{
	Perspective::rewind(path_id);
}


unsigned
PerspectiveTransformer::vertex(double* x, double* y)
{
#ifdef ICON_O_MATIC
	if (fValid)
		return Perspective::vertex(x, y);
	else
		return agg::path_cmd_stop;
#else
	return Perspective::vertex(x, y);
#endif
}


void
PerspectiveTransformer::SetSource(VertexSource& source)
{
	PathTransformer::SetSource(source);
	Perspective::attach(source);

#ifdef ICON_O_MATIC
	ObjectChanged(fShape);
#endif
}


double
PerspectiveTransformer::ApproximationScale() const
{
	return fSource.ApproximationScale() * scale();
}


// #pragma mark -


void
PerspectiveTransformer::Invert()
{
#ifdef ICON_O_MATIC
	fInverted = !fInverted;

	// TODO: degenerate matrices may not be adequately handled
	bool degenerate = !invert();
	fValid = fValid && !degenerate;
#else
	invert();
#endif
}


// #pragma mark -


#ifdef ICON_O_MATIC

status_t
PerspectiveTransformer::Archive(BMessage* into, bool deep) const
{
	status_t ret = Transformer::Archive(into, deep);

	into->what = archive_code;

	double matrix[9];
	store_to(matrix);

	for (int i = 0; i < 9; i++) {
		if (ret == B_OK)
			ret = into->AddDouble("matrix", matrix[i]);
	}

	return ret;
}


// #prama mark -


void
PerspectiveTransformer::ObjectChanged(const Observable* object)
{
	if (fInverted) {
		printf("calculating the validity or bounding box of an inverted "
			"perspective transformer is currently unsupported.");
		return;
	}

	uint32 pathID[1];
	pathID[0] = 0;
	double left, top, right, bottom;
	agg::bounding_rect(fSource, pathID, 0, 1, &left, &top, &right, &bottom);
	BRect newFromBox = BRect(left, top, right, bottom);

	// Stop if nothing we care about has changed
	// TODO: Can this be done earlier? It would be nice to avoid having to
	// recalculate the bounding box before realizing nothing needs to be done.
    if (fFromBox == newFromBox)
		return;

	fFromBox = newFromBox;

	_CheckValidity();

	double x = fFromBox.left; double y = fFromBox.top;
	Transform(&x, &y);
	fToLeftTop = BPoint(x, y);

	x = fFromBox.right; y = fFromBox.top;
	Transform(&x, &y);
	fToRightTop = BPoint(x, y);

	x = fFromBox.left; y = fFromBox.bottom;
	Transform(&x, &y);
	fToLeftBottom = BPoint(x, y);

	x = fFromBox.right; y = fFromBox.bottom;
	Transform(&x, &y);
	fToRightBottom = BPoint(x, y);
}


// #pragma mark -


void
PerspectiveTransformer::TransformTo(
	BPoint leftTop, BPoint rightTop, BPoint leftBottom, BPoint rightBottom)
{
	fToLeftTop = leftTop;
	fToRightTop = rightTop;
	fToLeftBottom = leftBottom;
	fToRightBottom = rightBottom;

	double quad[8] = {
		fToLeftTop.x, fToLeftTop.y,
		fToRightTop.x, fToRightTop.y,
		fToRightBottom.x, fToRightBottom.y,
		fToLeftBottom.x, fToLeftBottom.y
	};

	if (!fInverted) {
		rect_to_quad(
			fFromBox.left, fFromBox.top,
			fFromBox.right, fFromBox.bottom, quad);
	} else {
		quad_to_rect(quad,
			fFromBox.left, fFromBox.top,
			fFromBox.right, fFromBox.bottom);
	}

	_CheckValidity();
	Notify();
}


// #pragma mark -


void
PerspectiveTransformer::_CheckValidity()
{
	// Checks that none of the points are too close to the camera. These tend to
	// lead to very big numbers or a divide by zero error. Also checks that all
	// points are on the same side of the camera. Transformations with points on
	// different sides of the camera look weird and tend to cause crashes.

	fValid = true;
	double w;
	bool positive;

	if (!fInverted) {
		w = fFromBox.left * w0 + fFromBox.top * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		positive = w > 0;

		w = fFromBox.right * w0 + fFromBox.top * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;

		w = fFromBox.left * w0 + fFromBox.bottom * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;

		w = fFromBox.right * w0 + fFromBox.bottom * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;
	} else {
		w = fToLeftTop.x * w0 + fToLeftTop.y * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		positive = w > 0;

		w = fToRightTop.x * w0 + fToRightTop.y * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;

		w = fToLeftBottom.x * w0 + fToLeftBottom.y * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;

		w = fToRightBottom.x * w0 + fToRightBottom.y * w1 + w2;
		fValid &= (fabs(w) > 0.00001);
		fValid &= (w>0)==positive;
	}
}

#endif // ICON_O_MATIC
