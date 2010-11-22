/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SHAPE_CONTAINER_H
#define SHAPE_CONTAINER_H


#include <List.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


class Shape;

#ifdef ICON_O_MATIC
class ShapeContainerListener {
 public:
								ShapeContainerListener();
	virtual						~ShapeContainerListener();

	virtual	void				ShapeAdded(Shape* shape, int32 index) = 0;
	virtual	void				ShapeRemoved(Shape* shape) = 0;
};
#endif // ICON_O_MATIC

class ShapeContainer {
 public:
								ShapeContainer();
	virtual						~ShapeContainer();

			bool				AddShape(Shape* shape);
			bool				AddShape(Shape* shape, int32 index);
			bool				RemoveShape(Shape* shape);
			Shape*				RemoveShape(int32 index);

			void				MakeEmpty();

			int32				CountShapes() const;
			bool				HasShape(Shape* shape) const;
			int32				IndexOf(Shape* shape) const;

			Shape*				ShapeAt(int32 index) const;
			Shape*				ShapeAtFast(int32 index) const;

 private:
			void				_MakeEmpty();

			BList				fShapes;

#ifdef ICON_O_MATIC
 public:
			bool				AddListener(ShapeContainerListener* listener);
			bool				RemoveListener(
									ShapeContainerListener* listener);

 private:
			void				_NotifyShapeAdded(Shape* shape,
												  int32 index) const;
			void				_NotifyShapeRemoved(Shape* shape) const;

			BList				fListeners;
#endif // ICON_O_MATIC
};


_END_ICON_NAMESPACE


#endif	// SHAPE_CONTAINER_H
