#ifndef SHAPE_H
#define SHAPE_H

#include <List.h>
#include <Rect.h>

#include "Observable.h"
#include "PathContainer.h"
#include "PathSource.h"
#include "Referenceable.h"

class Style;

class Shape : public Observable,
			  public Referenceable,
			  public PathContainerListener {
 public:
								Shape(::Style* style);
	virtual						~Shape();

	// PathContainerListener interface
	virtual	void				PathAdded(VectorPath* path);
	virtual	void				PathRemoved(VectorPath* path);

	// Shape
			status_t			InitCheck() const;

	inline	PathContainer*		Paths() const
									{ return fPaths; }

			void				SetStyle(::Style* style);
	inline	::Style*			Style() const
									{ return fStyle; }

			BRect				Bounds() const;

			::VertexSource&		VertexSource();
			bool				AppendTransformer(
									Transformer* transformer);

 private:
			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;
};

#endif // SHAPE_H
