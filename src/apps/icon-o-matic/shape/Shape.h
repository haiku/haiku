#ifndef SHAPE_H
#define SHAPE_H

#include <List.h>
#include <Rect.h>

#include "IconObject.h"
#include "Observer.h"
#include "PathContainer.h"
#include "PathSource.h"
#include "Transformable.h"
#include "VectorPath.h"

class Style;

// TODO: merge Observer and ShapeListener interface
// ie add "AppearanceChanged(Shape* shape)"
class ShapeListener {
 public:
								ShapeListener();
	virtual						~ShapeListener();

	virtual	void				TransformerAdded(Transformer* t,
												 int32 index) = 0;
	virtual	void				TransformerRemoved(Transformer* t) = 0;

	virtual	void				StyleChanged(::Style* oldStyle,
											 ::Style* newStyle) = 0;
};

class Shape : public IconObject,
			  public Transformable,
			  public Observer,	// observing all the paths and the style
			  public PathContainerListener,
			  public PathListener {
 public:
								Shape(::Style* style);
								Shape(const Shape& other);
	virtual						~Shape();

	// Transformable interface
	virtual	void				TransformationChanged();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PathContainerListener interface
	virtual	void				PathAdded(VectorPath* path);
	virtual	void				PathRemoved(VectorPath* path);

	// PathListener interface
	virtual	void				PointAdded(int32 index);
	virtual	void				PointRemoved(int32 index);
	virtual	void				PointChanged(int32 index);
	virtual	void				PathChanged();
	virtual	void				PathClosedChanged();
	virtual	void				PathReversed();

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

			void				_NotifyStyleChanged(::Style* oldStyle,
													::Style* newStyle) const;

			void				_NotifyRerender() const;

			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;
	mutable	bool				fNeedsUpdate;

			BList				fListeners;

	mutable	BRect				fLastBounds;
};

#endif // SHAPE_H
