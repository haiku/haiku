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

// TODO: merge Observer and ShapeListener interface
class ShapeListener {
 public:
								ShapeListener();
	virtual						~ShapeListener();

	virtual	void				TransformerAdded(Transformer* t,
												 int32 index) = 0;
	virtual	void				TransformerRemoved(Transformer* t) = 0;
};

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

			void				SetName(const char* name);
			const char*			Name() const;

	inline	BRect				LastBounds() const
									{ return fLastBounds; }
			BRect				Bounds(bool updateLast = false) const;

			::VertexSource&		VertexSource();

			bool				AddTransformer(Transformer* transformer);
			bool				AddTransformer(Transformer* transformer,
											   int32 index);
			bool				RemoveTransformer(Transformer* transformer);

			int32				CountTransformers() const;

			bool				HasTransformer(Transformer* transformer) const;
			int32				IndexOf(Transformer* transformer) const;

			Transformer*		TransformerAt(int32 index) const;
			Transformer*		TransformerAtFast(int32 index) const;

			bool				AddListener(ShapeListener* listener);
			bool				RemoveListener(ShapeListener* listener);

 private:
			void				_NotifyTransformerAdded(Transformer* t,
														int32 index) const;
			void				_NotifyTransformerRemoved(Transformer* t) const;

			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;

			BList				fListeners;

	mutable	BRect				fLastBounds;

			BString				fName;
};

#endif // SHAPE_H
