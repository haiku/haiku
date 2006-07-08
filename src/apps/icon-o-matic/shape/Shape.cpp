#include "Shape.h"

#include <new>
#include <limits.h>
#include <stdio.h>

#include "agg_bounding_rect.h"

#include "Style.h"

using std::nothrow;

// constructor
ShapeListener::ShapeListener()
{
}

// destructor
ShapeListener::~ShapeListener()
{
}

// #pragma mark -

// constructor
Shape::Shape(::Style* style)
	: IconObject("<shape>"),
	  Observer(),
	  PathContainerListener(),

	  fPaths(new (nothrow) PathContainer(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(4),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1)
{
	if (fPaths)
		fPaths->AddListener(this);

	SetStyle(style);
}

// constructor
Shape::Shape(const Shape& other)
	: IconObject(other),
	  Observer(),
	  PathContainerListener(),

	  fPaths(new (nothrow) PathContainer(false)),
	  fStyle(NULL),

	  fPathSource(fPaths),
	  fTransformers(4),
	  fNeedsUpdate(true),

	  fLastBounds(0, 0, -1, -1)
{
	if (fPaths) {
		fPaths->AddListener(this);
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
	// TODO: clone vertex transformers

	SetStyle(other.fStyle);
}

// destructor
Shape::~Shape()
{
	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++) {
		Transformer* t = (Transformer*)fTransformers.ItemAtFast(i);
		t->RemoveObserver(this);
		delete t;
	}

	fPaths->MakeEmpty();
	fPaths->RemoveListener(this);
	delete fPaths;

	SetStyle(NULL);
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
Shape::PathAdded(VectorPath* path)
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

	if (fStyle) {
		fStyle->RemoveObserver(this);
		fStyle->Release();
	}

	::Style* oldStyle = fStyle;
	fStyle = style;

	if (fStyle) {
		fStyle->Acquire();
		fStyle->AddObserver(this);
	}

	_NotifyStyleChanged(oldStyle, fStyle);
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
	agg::bounding_rect(source, pathID, 0, 1,
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

	transformer->AddObserver(this);

	_NotifyTransformerAdded(transformer, index);
	return true;
}

// RemoveTransformer
bool
Shape::RemoveTransformer(Transformer* transformer)
{
	if (fTransformers.RemoveItem((void*)transformer)) {
		transformer->RemoveObserver(this);
		
		_NotifyTransformerRemoved(transformer);
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
