/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Shape.h"

#include <Message.h>
#include <TypeConstants.h>

#include <new>
#include <limits.h>
#include <stdio.h>

#include "agg_bounding_rect.h"

#ifdef ICON_O_MATIC
# include "CommonPropertyIDs.h"
# include "Property.h"
# include "PropertyObject.h"
#endif // ICON_O_MATIC
#include "Style.h"
#include "TransformerFactory.h"

using std::nothrow;

#ifdef ICON_O_MATIC
// constructor
ShapeListener::ShapeListener()
{
}

// destructor
ShapeListener::~ShapeListener()
{
}
#endif // ICON_O_MATIC

// #pragma mark -

// constructor
Shape::Shape(::Style* style)
#ifdef ICON_O_MATIC
	: IconObject("<shape>"),
	  Transformable(),
	  Observer(),
	  PathContainerListener(),
#else
	: Transformable(),
#endif

	  fPaths(new (nothrow) PathContainer(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(4),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1),

	  fHinting(false),
	  fMinVisibilityScale(0.0),
	  fMaxVisibilityScale(4.0)

#ifdef ICON_O_MATIC
	, fListeners(8)
#endif
{
	SetStyle(style);

#ifdef ICON_O_MATIC
	if (fPaths)
		fPaths->AddListener(this);
#endif
}

// constructor
Shape::Shape(const Shape& other)
#ifdef ICON_O_MATIC
	: IconObject(other),
	  Transformable(other),
	  Observer(),
	  PathContainerListener(),
#else
	: Transformable(other),
#endif

	  fPaths(new (nothrow) PathContainer(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(4),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1),

	  fHinting(other.fHinting),
	  fMinVisibilityScale(other.fMinVisibilityScale),
	  fMaxVisibilityScale(other.fMaxVisibilityScale)

#ifdef ICON_O_MATIC
	, fListeners(8)
#endif
{
	SetStyle(other.fStyle);

	if (fPaths) {
#ifdef ICON_O_MATIC
		fPaths->AddListener(this);
#endif
		// copy the path references from
		// the other shape
		if (other.fPaths) {
			int32 count = other.fPaths->CountPaths();
			for (int32 i = 0; i < count; i++) {
				if (!fPaths->AddPath(other.fPaths->PathAtFast(i)))
					break;
			}
		}
	}
	// clone vertex transformers
	int32 count = other.CountTransformers();
	for (int32 i = 0; i < count; i++) {
		Transformer* original = other.TransformerAtFast(i);
		Transformer* cloned = original->Clone(fPathSource);
		if (!AddTransformer(cloned)) {
			delete cloned;
			break;
		}
	}
}

// destructor
Shape::~Shape()
{
	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++) {
		Transformer* t = (Transformer*)fTransformers.ItemAtFast(i);
#ifdef ICON_O_MATIC
		t->RemoveObserver(this);
		_NotifyTransformerRemoved(t);
#endif
		delete t;
	}

	fPaths->MakeEmpty();
#ifdef ICON_O_MATIC
	fPaths->RemoveListener(this);
#endif
	delete fPaths;

	SetStyle(NULL);
}

// #pragma mark -

// Unarchive
status_t
Shape::Unarchive(const BMessage* archive)
{
#ifdef ICON_O_MATIC
	// IconObject properties
	status_t ret = IconObject::Unarchive(archive);
	if (ret < B_OK)
		return ret;
#else
	status_t ret;
#endif

	// recreate transformers
	BMessage transformerArchive;
	for (int32 i = 0;
		 archive->FindMessage("transformer", i,
		 					  &transformerArchive) == B_OK;
		 i++) {
		Transformer* transformer
			= TransformerFactory::TransformerFor(
				&transformerArchive, VertexSource());
		if (!transformer || !AddTransformer(transformer)) {
			delete transformer;
		}
	}

	// read transformation
	int32 size = Transformable::matrix_size;
	const void* matrix;
	ssize_t dataSize = size * sizeof(double);
	ret = archive->FindData("transformation", B_DOUBLE_TYPE,
							&matrix, &dataSize);
	if (ret == B_OK && dataSize == (ssize_t)(size * sizeof(double)))
		LoadFrom((const double*)matrix);

	// hinting
	if (archive->FindBool("hinting", &fHinting) < B_OK)
		fHinting = false;

	// min visibility scale
	if (archive->FindFloat("min visibility scale",
						   &fMinVisibilityScale) < B_OK)
		fMinVisibilityScale = 0.0;

	// max visibility scale
	if (archive->FindFloat("max visibility scale",
						   &fMaxVisibilityScale) < B_OK)
		fMaxVisibilityScale = 4.0;

	if (fMinVisibilityScale < 0.0)
		fMinVisibilityScale = 0.0;
	if (fMinVisibilityScale > 4.0)
		fMinVisibilityScale = 4.0;
	if (fMaxVisibilityScale < 0.0)
		fMaxVisibilityScale = 0.0;
	if (fMaxVisibilityScale > 4.0)
		fMaxVisibilityScale = 4.0;

	return B_OK;
}

#ifdef ICON_O_MATIC

// Archive
status_t
Shape::Archive(BMessage* into, bool deep) const
{
	status_t ret = IconObject::Archive(into, deep);

	// transformers
	if (ret == B_OK) {
		int32 count = CountTransformers();
		for (int32 i = 0; i < count; i++) {
			Transformer* transformer = TransformerAtFast(i);
			BMessage transformerArchive;
			ret = transformer->Archive(&transformerArchive);
			if (ret == B_OK)
				ret = into->AddMessage("transformer", &transformerArchive);
			if (ret < B_OK)
				break;
		}
	}

	// transformation
	if (ret == B_OK) {
		int32 size = Transformable::matrix_size;
		double matrix[size];
		StoreTo(matrix);
		ret = into->AddData("transformation", B_DOUBLE_TYPE,
							matrix, size * sizeof(double));
	}

	// hinting
	if (ret ==B_OK)
		ret = into->AddBool("hinting", fHinting);

	// min visibility scale
	if (ret ==B_OK)
		ret = into->AddFloat("min visibility scale",
							 fMinVisibilityScale);

	// max visibility scale
	if (ret ==B_OK)
		ret = into->AddFloat("max visibility scale",
							 fMaxVisibilityScale);

	return ret;
}

// MakePropertyObject
PropertyObject*
Shape::MakePropertyObject() const
{
	PropertyObject* object = IconObject::MakePropertyObject();
	if (!object)
		return NULL;

//	object->AddProperty(new BoolProperty(PROPERTY_HINTING, fHinting));

	object->AddProperty(new FloatProperty(PROPERTY_MIN_VISIBILITY_SCALE,
										  fMinVisibilityScale, 0, 4));

	object->AddProperty(new FloatProperty(PROPERTY_MAX_VISIBILITY_SCALE,
										  fMaxVisibilityScale, 0, 4));

	return object;
}

// SetToPropertyObject
bool
Shape::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	IconObject::SetToPropertyObject(object);

	// hinting
//	SetHinting(object->Value(PROPERTY_HINTING, fHinting));

	// min visibility scale
	SetMinVisibilityScale(object->Value(PROPERTY_MIN_VISIBILITY_SCALE,
										fMinVisibilityScale));

	// max visibility scale
	SetMaxVisibilityScale(object->Value(PROPERTY_MAX_VISIBILITY_SCALE,
										fMaxVisibilityScale));

	return HasPendingNotifications();
}

// #pragma mark -

// TransformationChanged
void
Shape::TransformationChanged()
{
	// TODO: notify appearance change
	_NotifyRerender();
}

// #pragma mark -

// ObjectChanged
void
Shape::ObjectChanged(const Observable* object)
{
	// simply pass on the event for now
	// (a path, transformer or the style changed,
	// the shape needs to be re-rendered)
	_NotifyRerender();
}

// #pragma mark -

// PathAdded
void
Shape::PathAdded(VectorPath* path, int32 index)
{
	path->Acquire();
	path->AddListener(this);
	_NotifyRerender();
}

// PathRemoved
void
Shape::PathRemoved(VectorPath* path)
{
	path->RemoveListener(this);
	_NotifyRerender();
	path->Release();
}

// #pragma mark -

// PointAdded
void
Shape::PointAdded(int32 index)
{
	_NotifyRerender();
}

// PointRemoved
void
Shape::PointRemoved(int32 index)
{
	_NotifyRerender();
}

// PointChanged
void
Shape::PointChanged(int32 index)
{
	_NotifyRerender();
}

// PathChanged
void
Shape::PathChanged()
{
	_NotifyRerender();
}

// PathClosedChanged
void
Shape::PathClosedChanged()
{
	_NotifyRerender();
}

// PathReversed
void
Shape::PathReversed()
{
	_NotifyRerender();
}

#endif // ICON_O_MATIC


// #pragma mark -

// InitCheck
status_t
Shape::InitCheck() const
{
	return fPaths ? B_OK : B_NO_MEMORY;
}

// #pragma mark -

// SetStyle
void
Shape::SetStyle(::Style* style)
{
	if (fStyle == style)
		return;

#ifdef ICON_O_MATIC
	if (fStyle) {
		fStyle->RemoveObserver(this);
		fStyle->Release();
	}
	::Style* oldStyle = fStyle;
#endif

	fStyle = style;

#ifdef ICON_O_MATIC
	if (fStyle) {
		fStyle->Acquire();
		fStyle->AddObserver(this);
	}

	_NotifyStyleChanged(oldStyle, fStyle);
#endif
}

// #pragma mark -

// Bounds
BRect
Shape::Bounds(bool updateLast) const
{
	// TODO: what about sub-paths?!?
	// the problem is that the path ids are
	// nowhere stored while converting VectorPath
	// to agg::path_storage, but it is also unclear
	// if those would mean anything later on in
	// the Transformer pipeline
	uint32 pathID[1];
	pathID[0] = 0;
	double left, top, right, bottom;

	::VertexSource& source = const_cast<Shape*>(this)->VertexSource();
	agg::conv_transform< ::VertexSource, Transformable>
			transformedSource(source, *this);
	agg::bounding_rect(transformedSource, pathID, 0, 1,
					   &left, &top, &right, &bottom);

	BRect bounds(left, top, right, bottom);

	if (updateLast)
		fLastBounds = bounds;

	return bounds;
}

// VertexSource
::VertexSource&
Shape::VertexSource()
{
	::VertexSource* source = &fPathSource;

	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++) {
		Transformer* t = (Transformer*)fTransformers.ItemAtFast(i);
		t->SetSource(*source);
		source = t;
	}

	if (fNeedsUpdate) {
		fPathSource.Update(source->WantsOpenPaths(),
						   source->ApproximationScale());
		fNeedsUpdate = false;
	}

	return *source;
}

// SetGlobalScale
void
Shape::SetGlobalScale(double scale)
{
	fPathSource.SetGlobalScale(scale);
}

// AddTransformer
bool
Shape::AddTransformer(Transformer* transformer)
{
	return AddTransformer(transformer, CountTransformers());
}

// AddTransformer
bool
Shape::AddTransformer(Transformer* transformer, int32 index)
{
	if (!transformer)
		return false;

	if (!fTransformers.AddItem((void*)transformer, index))
		return false;

#ifdef ICON_O_MATIC
	transformer->AddObserver(this);

	_NotifyTransformerAdded(transformer, index);
#else
	fNeedsUpdate = true;
#endif
	return true;
}

// RemoveTransformer
bool
Shape::RemoveTransformer(Transformer* transformer)
{
	if (fTransformers.RemoveItem((void*)transformer)) {
#ifdef ICON_O_MATIC
		transformer->RemoveObserver(this);
		
		_NotifyTransformerRemoved(transformer);
#else
		fNeedsUpdate = true;
#endif
		return true;
	}

	return false;
}

// #pragma mark -

// CountShapes
int32
Shape::CountTransformers() const
{
	return fTransformers.CountItems();
}

// HasTransformer
bool
Shape::HasTransformer(Transformer* transformer) const
{
	return fTransformers.HasItem((void*)transformer);
}

// IndexOf
int32
Shape::IndexOf(Transformer* transformer) const
{
	return fTransformers.IndexOf((void*)transformer);
}

// TransformerAt
Transformer*
Shape::TransformerAt(int32 index) const
{
	return (Transformer*)fTransformers.ItemAt(index);
}

// TransformerAtFast
Transformer*
Shape::TransformerAtFast(int32 index) const
{
	return (Transformer*)fTransformers.ItemAtFast(index);
}

// #pragma mark -

// SetHinting
void
Shape::SetHinting(bool hinting)
{
	if (fHinting == hinting)
		return;

	fHinting = hinting;
	Notify();
}

// SetMinVisibilityScale
void
Shape::SetMinVisibilityScale(float scale)
{
	if (fMinVisibilityScale == scale)
		return;

	fMinVisibilityScale = scale;
	Notify();
}

// SetMaxVisibilityScale
void
Shape::SetMaxVisibilityScale(float scale)
{
	if (fMaxVisibilityScale == scale)
		return;

	fMaxVisibilityScale = scale;
	Notify();
}

// #pragma mark -

#ifdef ICON_O_MATIC

// AddListener
bool
Shape::AddListener(ShapeListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}

// RemoveListener
bool
Shape::RemoveListener(ShapeListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}

// #pragma mark -

// _NotifyTransformerAdded
void
Shape::_NotifyTransformerAdded(Transformer* transformer, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListener* listener
			= (ShapeListener*)listeners.ItemAtFast(i);
		listener->TransformerAdded(transformer, index);
	}
	// TODO: merge Observable and ShapeListener interface
	_NotifyRerender();
}

// _NotifyTransformerRemoved
void
Shape::_NotifyTransformerRemoved(Transformer* transformer) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListener* listener
			= (ShapeListener*)listeners.ItemAtFast(i);
		listener->TransformerRemoved(transformer);
	}
	// TODO: merge Observable and ShapeListener interface
	_NotifyRerender();
}

// _NotifyStyleChanged
void
Shape::_NotifyStyleChanged(::Style* oldStyle, ::Style* newStyle) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeListener* listener
			= (ShapeListener*)listeners.ItemAtFast(i);
		listener->StyleChanged(oldStyle, newStyle);
	}
	// TODO: merge Observable and ShapeListener interface
	_NotifyRerender();
}

// _NotifyRerender
void
Shape::_NotifyRerender() const
{
	fNeedsUpdate = true;
	Notify();
}

#endif // ICON_O_MATIC

