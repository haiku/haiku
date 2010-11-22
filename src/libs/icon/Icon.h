/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ICON_H
#define ICON_H


#include "IconBuild.h"
#include "ShapeContainer.h"

#ifdef ICON_O_MATIC
#	include <List.h>

#	include "Observer.h"
#	include "Referenceable.h"
#else
#	include <SupportDefs.h>
#endif

class BRect;


_BEGIN_ICON_NAMESPACE


class PathContainer;
class StyleContainer;

#ifdef ICON_O_MATIC
class IconListener {
 public:
								IconListener();
	virtual						~IconListener();

	virtual	void				AreaInvalidated(const BRect& area) = 0;
};
#endif

#ifdef ICON_O_MATIC
class Icon : public ShapeContainerListener,
			 public Observer,
			 public Referenceable {
#else
class Icon {
#endif

 public:
								Icon();
								Icon(const Icon& other);
	virtual						~Icon();

			status_t			InitCheck() const;

			StyleContainer*		Styles() const
									{ return fStyles; }
			PathContainer*		Paths() const
									{ return fPaths; }
			ShapeContainer*		Shapes() const
									{ return fShapes; }

			Icon*				Clone() const;
			void				MakeEmpty();

 private:

			StyleContainer*		fStyles;
			PathContainer*		fPaths;
			ShapeContainer*		fShapes;

#ifdef ICON_O_MATIC
 public:
	// ShapeContainerListener interface
	virtual	void				ShapeAdded(Shape* shape, int32 index);
	virtual	void				ShapeRemoved(Shape* shape);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Icon
			bool				AddListener(IconListener* listener);
			bool				RemoveListener(IconListener* listener);

 private:
			void				_NotifyAreaInvalidated(
									const BRect& area) const;
			BList				fListeners;
#endif
};


_END_ICON_NAMESPACE


#endif	// ICON_H
