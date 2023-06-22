/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ICON_H
#define ICON_H


#include "Container.h"
#include "IconBuild.h"

#ifdef ICON_O_MATIC
#	include <List.h>
#	include <Referenceable.h>

#	include "Observer.h"
#else
#	include <SupportDefs.h>
#endif

class BRect;


_BEGIN_ICON_NAMESPACE


class Shape;
class Style;
class VectorPath;

#ifdef ICON_O_MATIC
class IconListener {
 public:
								IconListener();
	virtual						~IconListener();

	virtual	void				AreaInvalidated(const BRect& area) = 0;
};
#endif

#ifdef ICON_O_MATIC
class Icon : public ContainerListener<Shape>,
			 public Observer,
			 public BReferenceable {
#else
class Icon {
#endif

 public:
									Icon();
									Icon(const Icon& other);
	virtual							~Icon();

			status_t				InitCheck() const;

			const Container<Style>*	Styles() const
										{ return &fStyles; }
			Container<Style>*		Styles()
										{ return &fStyles; }
			const Container<VectorPath>*	Paths() const
										{ return &fPaths; }
			Container<VectorPath>*	Paths()
										{ return &fPaths; }
			const Container<Shape>*	Shapes() const
										{ return &fShapes; }
			Container<Shape>*		Shapes()
										{ return &fShapes; }

			Icon*					Clone() const;
			void					MakeEmpty();

 private:

			Container<Style>		fStyles;
			Container<VectorPath>	fPaths;
			Container<Shape>		fShapes;

#ifdef ICON_O_MATIC
 public:
	// ContainerListener<Shape> interface
	virtual	void					ItemAdded(Shape* shape, int32 index);
	virtual	void					ItemRemoved(Shape* shape);

	// Observer interface
	virtual	void					ObjectChanged(const Observable* object);

	// Icon
			bool					AddListener(IconListener* listener);
			bool					RemoveListener(IconListener* listener);

 private:
			void					_NotifyAreaInvalidated(
										const BRect& area) const;
			BList					fListeners;
#endif
};


_END_ICON_NAMESPACE


#endif	// ICON_H
