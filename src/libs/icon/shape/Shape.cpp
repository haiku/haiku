/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
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
#include "Container.h"
#include "PathTransformer.h"
#include "Style.h"
#include "TransformerFactory.h"
#include "VectorPath.h"

using std::nothrow;


#ifdef ICON_O_MATIC
ShapeListener::ShapeListener()
{
}


ShapeListener::~ShapeListener()
{
}
#endif // ICON_O_MATIC


// #pragma mark -


Shape::Shape(::Style* style)
#ifdef ICON_O_MATIC
	: IconObject("<shape>"),
	  Transformable(),
	  Observer(),
	  ContainerListener<VectorPath>(),
#else
	: Transformable(),
#endif

	  fPaths(new (nothrow) Container<VectorPath>(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(true),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1),

	  fHinting(false)

#ifdef ICON_O_MATIC
	, fListeners(8)
#endif
{
	SetStyle(style);

	fTransformers.AddListener(this);

#ifdef ICON_O_MATIC
	if (fPaths)
		fPaths->AddListener(this);
#endif
}


Shape::Shape(const Shape& other)
#ifdef ICON_O_MATIC
	: IconObject(other),
	  Transformable(other),
	  Observer(),
	  ContainerListener<VectorPath>(),
#else
	: Transformable(other),
#endif

	  fPaths(new (nothrow) Container<VectorPath>(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(true),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1),

	  fHinting(false)

#ifdef ICON_O_MATIC
	, fListeners(8)
#endif
{
	SetStyle(other.fStyle);

	fTransformers.AddListener(this);

	if (fPaths) {
#ifdef ICON_O_MATIC
		fPaths->AddListener(this);
#endif

		// copy the path references from
		// the other shape
		if (other.fPaths) {
			int32 count = other.fPaths->CountItems();
			for (int32 i = 0; i < count; i++) {
				if (!fPaths->AddItem(other.fPaths->ItemAtFast(i)))
					break;
			}
		}
	}
	// clone vertex transformers
	int32 count = other.Transformers()->CountItems();
	for (int32 i = 0; i < count; i++) {
		Transformer* original = other.Transformers()->ItemAtFast(i);
		Transformer* cloned = original->Clone();
		if (!fTransformers.AddItem(cloned)) {
			delete cloned;
			break;
		}
	}
}


Shape::~Shape()
{
	fPaths->MakeEmpty();
#ifdef ICON_O_MATIC
	fPaths->RemoveListener(this);
#endif
	delete fPaths;

	fTransformers.MakeEmpty();
	fTransformers.RemoveListener(this);

	SetStyle(NULL);
}


// #pragma mark -


status_t
Shape::Unarchive(BMessage* archive)
{
#ifdef ICON_O_MATIC
	// IconObject properties
	status_t ret = IconObject::Unarchive(archive);
	if (ret < B_OK)
		return ret;
#else
	status_t ret;
#endif

	// hinting
	if (archive->FindBool("hinting", &fHinting) < B_OK)
		fHinting = false;

	// recreate transformers
	BMessage transformerArchive;
	for (int32 i = 0;
		 archive->FindMessage("transformer", i,
			 &transformerArchive) == B_OK;
		 i++) {
		Transformer* transformer
			= TransformerFactory::TransformerFor(
				&transformerArchive, VertexSource(), this);
		if (!transformer || !fTransformers.AddItem(transformer)) {
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

	return B_OK;
}


#ifdef ICON_O_MATIC
status_t
Shape::Archive(BMessage* into, bool deep) const
{
	status_t ret = IconObject::Archive(into, deep);

	// hinting
	if (ret == B_OK)
		ret = into->AddBool("hinting", fHinting);

	// transformers
	if (ret == B_OK) {
		int32 count = fTransformers.CountItems();
		for (int32 i = 0; i < count; i++) {
			Transformer* transformer = fTransformers.ItemAtFast(i);
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

	return ret;
}


PropertyObject*
Shape::MakePropertyObject() const
{
	PropertyObject* object = IconObject::MakePropertyObject();
	return object;
}


bool
Shape::SetToPropertyObject(const PropertyObject* object)
{
	IconObject::SetToPropertyObject(object);
	return true;
}


// #pragma mark -


void
Shape::TransformationChanged()
{
	// TODO: notify appearance change
	_NotifyRerender();
}


// #pragma mark -


void
Shape::ObjectChanged(const Observable* object)
{
	// simply pass on the event for now
	// (a path, transformer or the style changed,
	// the shape needs to be re-rendered)
	_NotifyRerender();
}


// #pragma mark -


void
Shape::ItemAdded(VectorPath* path, int32 index)
{
	path->AcquireReference();
	path->AddListener(this);
	_NotifyRerender();
}


void
Shape::ItemRemoved(VectorPath* path)
{
	path->RemoveListener(this);
	_NotifyRerender();
	path->ReleaseReference();
}


// #pragma mark -


void
Shape::PointAdded(int32 index)
{
	_NotifyRerender();
}


void
Shape::PointRemoved(int32 index)
{
	_NotifyRerender();
}


void
Shape::PointChanged(int32 index)
{
	_NotifyRerender();
}


void
Shape::PathChanged()
{
	_NotifyRerender();
}


void
Shape::PathClosedChanged()
{
	_NotifyRerender();
}


void
Shape::PathReversed()
{
	_NotifyRerender();
}
#endif // ICON_O_MATIC


// #pragma mark -


void
Shape::ItemAdded(Transformer* transformer, int32 index)
{
#ifdef ICON_O_MATIC
	transformer->AddObserver(this);

	// TODO: merge Observable and ShapeListener interface
	_NotifyRerender();
#else
	fNeedsUpdate = true;
#endif
}


void
Shape::ItemRemoved(Transformer* transformer)
{
#ifdef ICON_O_MATIC
	transformer->RemoveObserver(this);

	_NotifyRerender();
#else
	fNeedsUpdate = true;
#endif
}


// #pragma mark -


status_t
Shape::InitCheck() const
{
	return fPaths ? B_OK : B_NO_MEMORY;
}


// #pragma mark -


void
Shape::SetStyle(::Style* style)
{
	if (fStyle == style)
		return;

#ifdef ICON_O_MATIC
	if (fStyle) {
		fStyle->RemoveObserver(this);
		fStyle->ReleaseReference();
	}
	::Style* oldStyle = fStyle;
#endif

	fStyle = style;

#ifdef ICON_O_MATIC
	if (fStyle) {
		fStyle->AcquireReference();
		fStyle->AddObserver(this);
	}

	_NotifyStyleChanged(oldStyle, fStyle);
#endif
}


// #pragma mark -


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


::VertexSource&
Shape::VertexSource()
{
	::VertexSource* source = &fPathSource;

	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PathTransformer* t = dynamic_cast<PathTransformer*>(fTransformers.ItemAtFast(i));
		if (t != NULL) {
			t->SetSource(*source);
			source = t;
		}
	}

	if (fNeedsUpdate) {
		fPathSource.Update(source->WantsOpenPaths(),
			source->ApproximationScale());
		fNeedsUpdate = false;
	}

	return *source;
}


void
Shape::SetGlobalScale(double scale)
{
	fPathSource.SetGlobalScale(scale);
}


// #pragma mark -


#ifdef ICON_O_MATIC
bool
Shape::AddListener(ShapeListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}


bool
Shape::RemoveListener(ShapeListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}


// #pragma mark -


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


void
Shape::_NotifyRerender() const
{
	fNeedsUpdate = true;
	Notify();
}
#endif // ICON_O_MATIC


void
Shape::SetHinting(bool hinting)
{
	if (fHinting == hinting)
		return;

	fHinting = hinting;
	Notify();
}

