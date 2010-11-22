/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SHAPE_H
#define SHAPE_H


#ifdef ICON_O_MATIC
#	include "IconObject.h"
#	include "Observer.h"
#endif
#include "IconBuild.h"
#include "PathContainer.h"
#include "PathSource.h"
#include "Transformable.h"
#include "VectorPath.h"

#include <List.h>
#include <Rect.h>


_BEGIN_ICON_NAMESPACE


class Style;

#ifdef ICON_O_MATIC
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
#endif // ICON_O_MATIC

#ifdef ICON_O_MATIC
class Shape : public IconObject,
			  public _ICON_NAMESPACE Transformable,
			  public Observer,	// observing all the paths and the style
			  public PathContainerListener,
			  public PathListener {
#else
class Shape : public _ICON_NAMESPACE Transformable {
#endif

 public:
								Shape(::Style* style);
								Shape(const Shape& other);
	virtual						~Shape();

	// IconObject interface
	virtual	status_t			Unarchive(const BMessage* archive);
#ifdef ICON_O_MATIC
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);

	// Transformable interface
	virtual	void				TransformationChanged();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PathContainerListener interface
	virtual	void				PathAdded(VectorPath* path, int32 index);
	virtual	void				PathRemoved(VectorPath* path);

	// PathListener interface
	virtual	void				PointAdded(int32 index);
	virtual	void				PointRemoved(int32 index);
	virtual	void				PointChanged(int32 index);
	virtual	void				PathChanged();
	virtual	void				PathClosedChanged();
	virtual	void				PathReversed();
#else
	inline	void				Notify() {}
#endif // ICON_O_MATIC

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
			void				SetGlobalScale(double scale);

			bool				AddTransformer(Transformer* transformer);
			bool				AddTransformer(Transformer* transformer,
											   int32 index);
			bool				RemoveTransformer(Transformer* transformer);

			int32				CountTransformers() const;

			bool				HasTransformer(Transformer* transformer) const;
			int32				IndexOf(Transformer* transformer) const;

			Transformer*		TransformerAt(int32 index) const;
			Transformer*		TransformerAtFast(int32 index) const;

			void				SetHinting(bool hinting);
			bool				Hinting() const
									{ return fHinting; }
			void				SetMinVisibilityScale(float scale);
			float				MinVisibilityScale() const
									{ return fMinVisibilityScale; }
			void				SetMaxVisibilityScale(float scale);
			float				MaxVisibilityScale() const
									{ return fMaxVisibilityScale; }

#ifdef ICON_O_MATIC
			bool				AddListener(ShapeListener* listener);
			bool				RemoveListener(ShapeListener* listener);

 private:
			void				_NotifyTransformerAdded(Transformer* t,
														int32 index) const;
			void				_NotifyTransformerRemoved(Transformer* t) const;

			void				_NotifyStyleChanged(::Style* oldStyle,
													::Style* newStyle) const;

			void				_NotifyRerender() const;
#endif // ICON_O_MATIC

 private:
			PathContainer*		fPaths;
			::Style*			fStyle;

			PathSource			fPathSource;
			BList				fTransformers;
	mutable	bool				fNeedsUpdate;

	mutable	BRect				fLastBounds;

			bool				fHinting;
			float				fMinVisibilityScale;
			float				fMaxVisibilityScale;

#ifdef ICON_O_MATIC
			BList				fListeners;
#endif
};


_END_ICON_NAMESPACE


#endif	// SHAPE_H
