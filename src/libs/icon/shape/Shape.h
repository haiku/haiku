/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef SHAPE_H
#define SHAPE_H


#ifdef ICON_O_MATIC
#	include "IconObject.h"
#	include "Observer.h"
#endif
#include "Container.h"
#include "IconBuild.h"
#include "PathSource.h"
#include "Transformable.h"
#include "VectorPath.h"

#include <List.h>
#include <Rect.h>


class BMessage;

_BEGIN_ICON_NAMESPACE

class Style;


#ifdef ICON_O_MATIC
// TODO: merge Observer and ShapeListener interface
// ie add "AppearanceChanged(Shape* shape)"
class ShapeListener {
 public:
								ShapeListener();
	virtual						~ShapeListener();

	// TODO: this is not useful for all subclasses of Shape (e.g. ReferenceImage)
	virtual	void				StyleChanged(Style* oldStyle,
											 Style* newStyle) = 0;
};
#endif // ICON_O_MATIC

#ifdef ICON_O_MATIC
class Shape : public IconObject,
			  public _ICON_NAMESPACE Transformable,
			  public Observer,	// observing all the paths and the style
			  public ContainerListener<VectorPath>,
			  public ContainerListener<Transformer>,
			  public PathListener {
#else
class Shape : public _ICON_NAMESPACE Transformable,
			  public ContainerListener<Transformer> {
#endif

 public:
									Shape(_ICON_NAMESPACE Style* style);
									Shape(const Shape& other);
	virtual							~Shape();

	// IconObject interface
	virtual	status_t				Unarchive(BMessage* archive);
#ifdef ICON_O_MATIC
	virtual	status_t				Archive(BMessage* into,
											bool deep = true) const;

	virtual	PropertyObject*			MakePropertyObject() const;
	virtual	bool					SetToPropertyObject(
										const PropertyObject* object);

	// Transformable interface
	virtual	void					TransformationChanged();

	// Observer interface
	virtual	void					ObjectChanged(const Observable* object);

	// ContainerListener<VectorPath> interface
	virtual	void					ItemAdded(VectorPath* path, int32 index);
	virtual	void					ItemRemoved(VectorPath* path);

	// PathListener interface
	virtual	void					PointAdded(int32 index);
	virtual	void					PointRemoved(int32 index);
	virtual	void					PointChanged(int32 index);
	virtual	void					PathChanged();
	virtual	void					PathClosedChanged();
	virtual	void					PathReversed();
#else
	inline	void					Notify() {}
#endif // ICON_O_MATIC

	// ContainerListener<Transformer> interface
	virtual	void					ItemAdded(Transformer* t, int32 index);
	virtual	void					ItemRemoved(Transformer* t);

	// Shape
	virtual	status_t				InitCheck() const;
	virtual Shape*					Clone() const = 0;

	inline	Container<VectorPath>*	Paths() const
									{ return fPaths; }
	const	Container<Transformer>*	Transformers() const
									{ return &fTransformers; }
			Container<Transformer>* Transformers()
									{ return &fTransformers; }

      public:
	inline	_ICON_NAMESPACE Style*	Style() const
										{ return fStyle; }

	inline	BRect					LastBounds() const
										{ return fLastBounds; }
			BRect					Bounds(bool updateLast = false) const;

			_ICON_NAMESPACE VertexSource&	VertexSource();
			void					SetGlobalScale(double scale);

			void					SetHinting(bool hinting);
			bool					Hinting() const
										{ return fHinting; }

	virtual bool					Visible(float scale) const = 0;

#ifdef ICON_O_MATIC
			bool					AddListener(ShapeListener* listener);
			bool					RemoveListener(ShapeListener* listener);

 private:
			void					_NotifyStyleChanged(_ICON_NAMESPACE Style* oldStyle,
														_ICON_NAMESPACE Style* newStyle) const;

			void					_NotifyRerender() const;
#endif // ICON_O_MATIC

 protected:
			void					SetStyle(_ICON_NAMESPACE Style* style);

 private:
			Container<VectorPath>*	fPaths;
			_ICON_NAMESPACE Style*	fStyle;

			PathSource				fPathSource;
			Container<Transformer>	fTransformers;
	mutable	bool					fNeedsUpdate;

	mutable	BRect					fLastBounds;

			bool					fHinting;

#ifdef ICON_O_MATIC
			BList					fListeners;
#endif
};

_END_ICON_NAMESPACE


#endif	// SHAPE_H
