/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#ifdef ICON_O_MATIC
#include "ReferenceImage.h"

#include <algorithm>
#include <new>

#include <Bitmap.h>
#include <Message.h>

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "PropertyObject.h"
#include "Style.h"

using std::nothrow;


ReferenceImage::ReferenceImage(BBitmap* image)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	if (Style() == NULL)
		return;
	Style()->ReleaseReference();
		// The shape constructor acquired a reference

	SetName("<reference image>");
	SetImage(image);

	double width = (int) Style()->Bitmap()->Bounds().Width() + 1;
	double height = (int) Style()->Bitmap()->Bounds().Height() + 1;

	// Scale to fill canvas
	double longerSide = std::max(width, height);
	ScaleBy(BPoint(0, 0), 64/longerSide, 64/longerSide);

	// Center
	Transform(&width, &height);
	TranslateBy(BPoint((64-width)/2, (64-height)/2));
}


ReferenceImage::ReferenceImage(const ReferenceImage& other)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	if (Style() == NULL)
		return;
	Style()->ReleaseReference();
		// The shape constructor acquired a reference

	BBitmap* bitmap = new (nothrow) BBitmap(other.Style()->Bitmap());
	if (bitmap == NULL)
		return;

	SetName(other.Name());
	SetImage(bitmap);
	SetTransform(other);
	Style()->SetAlpha(other.Style()->Alpha());
}


ReferenceImage::ReferenceImage(BMessage* archive)
	: Shape(new (nothrow) _ICON_NAMESPACE Style()),
	fPath(NULL)
{
	Unarchive(archive);
}


ReferenceImage::~ReferenceImage()
{
}


// #pragma mark -


status_t
ReferenceImage::Unarchive(BMessage* archive)
{
	// IconObject properties
	status_t ret = IconObject::Unarchive(archive);
	if (ret < B_OK)
		return ret;

	// transformation
	const double* matrix;
	ssize_t dataSize;
	ret = archive->FindData("transformation", B_DOUBLE_TYPE,
		(const void**) &matrix, &dataSize);
	if (ret < B_OK)
		return ret;
	if (dataSize != Transformable::matrix_size * sizeof(double))
		return B_BAD_VALUE;
	LoadFrom(matrix);

	// image
	BBitmap* bitmap = dynamic_cast<BBitmap*>(BBitmap::Instantiate(archive));
	if (bitmap == NULL)
		return B_ERROR;
	SetImage(bitmap);

	// alpha
	uint8 alpha;
	if(archive->FindUInt8("alpha", &alpha) < B_OK)
		alpha = 255;
	Style()->SetAlpha(alpha);

	return B_OK;
}


status_t
ReferenceImage::Archive(BMessage* into, bool deep) const
{
	status_t ret = IconObject::Archive(into, deep);
	if (ret < B_OK)
		return ret;

	// transformation
	int32 size = Transformable::matrix_size;
	double matrix[size];
	StoreTo(matrix);
	ret = into->AddData("transformation", B_DOUBLE_TYPE,
		matrix, size * sizeof(double));
	if (ret < B_OK)
		return ret;

	// image
	ret = Style()->Bitmap()->Archive(into, deep);
	if (ret < B_OK)
		return ret;

	// alpha
	ret = into->AddUInt8("alpha", Style()->Alpha());

	return ret;
}


PropertyObject*
ReferenceImage::MakePropertyObject() const
{
	PropertyObject* object = IconObject::MakePropertyObject();

	object->AddProperty(new IntProperty(PROPERTY_ALPHA, Style()->Alpha(), 0, 255));

	return object;
}


bool
ReferenceImage::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	IconObject::SetToPropertyObject(object);

	Style()->SetAlpha(object->Value(PROPERTY_ALPHA, (int32) Style()->Alpha()));

	return HasPendingNotifications();
}


// #pragma mark -


status_t
ReferenceImage::InitCheck() const
{
	status_t status = Shape::InitCheck();
	if (status != B_OK)
		return status;

	if (Style() == NULL || Style()->Bitmap() == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


// #pragma mark -


void
ReferenceImage::SetImage(BBitmap* image)
{
	if (fPath != NULL) {
		Paths()->MakeEmpty();
		delete fPath;
		fPath = NULL;
	}

	if (image != NULL) {
		// Update style
		Style()->SetBitmap(image);

		// Update shape
		fPath = new (nothrow) VectorPath();
		if (fPath == NULL)
			return;

		double width = (int) Style()->Bitmap()->Bounds().Width() + 1;
		double height = (int) Style()->Bitmap()->Bounds().Height() + 1;
		fPath->AddPoint(BPoint(0, 0));
		fPath->AddPoint(BPoint(0, height));
		fPath->AddPoint(BPoint(width, height));
		fPath->AddPoint(BPoint(width, 0));
		fPath->SetClosed(true);
		Paths()->AddPath(fPath);
	}
}

#endif // ICON_O_MATIC

