#ifndef SHAPE_H
#define SHAPE_H

#include <List.h>
#include <Rect.h>

#include "Observable.h"
#include "Observer.h"
#include "PathContainer.h"
#include "PathSource.h"
#include "Referenceable.h"

class Style;

class Shape : public Observable,
			  public Observer,	// observing all the paths
			  public Referenceable,
			  public PathContainerListener {
 public:
								Shape(::Style* style);
	virtual						~Shape();

	// PathContainerListener interface
	virtual	void				PathAdded(VectorPath* path);
	virtual	void				PathRemoved(VectorPath* path);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

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

 private:
			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;

	mutable	BRect				fLastBounds;
};

#endif // SHAPE_H
