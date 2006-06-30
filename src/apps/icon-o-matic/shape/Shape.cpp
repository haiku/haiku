#include "Shape.h"

#include <new>
#include <limits.h>
#include <stdio.h>

#include "agg_bounding_rect.h"

#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
Shape::Shape(::Style* style)
	: Observable(),
	  Referenceable(),
	  PathContainerListener(),

	  fPaths(new (nothrow) PathContainer(false)),
	  fStyle(style),

	  fPathSource(fPaths),
	  fTransformers(4),

	  fLastBounds(0, 0, -1, -1),

	  fName("<shape>")
{
	if (fPaths)
		fPaths->AddListener(this);

	if (fStyle)
		fStyle->Acquire();
}

// destructor
Shape::~Shape()
{
	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (Transformer*)fTransformers.ItemAtFast(i);

	fPaths->MakeEmpty();
	fPaths->RemoveListener(this);
	delete fPaths;

	if (fStyle)
		fStyle->Release();
}

// PathAdded
void
Shape::PathAdded(VectorPath* path)
{
	path->Acquire();
	path->AddObserver(this);
	Notify();
}

// PathRemoved
void
Shape::PathRemoved(VectorPath* path)
{
	path->RemoveObserver(this);
	Notify();
	path->Release();
}

// #pragma mark -

// ObjectChanged
void
Shape::ObjectChanged(const Observable* object)
{
	// simply pass on the event for now
	Notify();
}

// #pragma mark -

// SelectedChanged
void
Shape::SelectedChanged()
{
	// simply pass on the event for now
	Notify();
}

// #pragma mark -

// InitCheck
status_t
Shape::InitCheck() const
{
	return fPaths ? B_OK : B_NO_MEMORY;
}

// SetStyle
void
Shape::SetStyle(::Style* style)
{
	if (fStyle == style)
		return;

	if (fStyle)
		fStyle->Release();

	fStyle = style;

	if (fStyle)
		fStyle->Acquire();

	Notify();
}

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
	fPathSource.Update();
	::VertexSource* source = &fPathSource;

	int32 count = fTransformers.CountItems();
	for (int32 i = 0; i < count; i++) {
		Transformer* t = (Transformer*)fTransformers.ItemAtFast(i);
		t->SetSource(*source);
		source = t;
	}

	return *source;
}

// AppendTransformer
bool
Shape::AppendTransformer(Transformer* transformer)
{
	if (!transformer)
		return false;

	if (!fTransformers.AddItem((void*)transformer))
		return false;

	Notify();
	return true;
}

// #pragma mark -

// SetName
void
Shape::SetName(const char* name)
{
	if (fName == name)
		return;

	fName = name;
	Notify();
}

// Name
const char*
Shape::Name() const
{
	return fName.String();
}

