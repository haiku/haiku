/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_H
#define ICON_H

#include <List.h>

#include "Observer.h"
#include "ShapeContainer.h"

class PathContainer;

class IconListener {
 public:
								IconListener();
	virtual						~IconListener();

	virtual	void				AreaInvalidated(const BRect& area) = 0;
};

class Icon : public ShapeContainerListener,
			 public Observer {
 public:
								Icon();
	virtual						~Icon();

	// ShapeContainerListener interface
	virtual	void				ShapeAdded(Shape* shape);
	virtual	void				ShapeRemoved(Shape* shape);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Icon
			PathContainer*		Paths() const
									{ return fPaths; }
			ShapeContainer*		Shapes() const
									{ return fShapes; }

			bool				AddListener(IconListener* listener);
			bool				RemoveListener(IconListener* listener);

 private:
			void				_NotifyAreaInvalidated(
									const BRect& area) const;

			PathContainer*		fPaths;
			ShapeContainer*		fShapes;

			BList				fListeners;
};

#endif // ICON_H
