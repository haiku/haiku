#ifndef SHAPE_H
#define SHAPE_H

#include <List.h>
#include <Rect.h>
#include <String.h>

#include "Observable.h"
#include "Observer.h"
#include "PathContainer.h"
#include "PathSource.h"
#include "Referenceable.h"
#include "Selectable.h"

class Style;

class Shape : public Observable,
			  public Observer,	// observing all the paths
			  public Referenceable,
			  public Selectable,
			  public PathContainerListener {
 public:
								Shape(::Style* style);
								Shape(const Shape& other);
	virtual						~Shape();

	// PathContainerListener interface
	virtual	void				PathAdded(VectorPath* path);
	virtual	void				PathRemoved(VectorPath* path);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Selectable interface
	virtual	void				SelectedChanged();

	// Shape
			status_t			InitCheck() const;

	inline	PathContainer*		Paths() const
									{ return fPaths; }

			void				SetStyle(::Style* style);
	inline	::Style*			Style() const
									{ return fStyle; }

	inline	BRect				LastBounds() const
									{ return fLastBounds; }
			BRect				Bounds(bool updateLast = false) const;

			::VertexSource&		VertexSource();
			bool				AppendTransformer(
									Transformer* transformer);

			void				SetName(const char* name);
			const char*			Name() const;

 private:
			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;

	mutable	BRect				fLastBounds;

			BString				fName;
};

#endif // SHAPE_H
