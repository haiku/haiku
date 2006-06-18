/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_H
#define ICON_H

class PathContainer;
class ShapeContainer;

class Icon {
 public:
								Icon();
	virtual						~Icon();

			PathContainer*		Paths() const
									{ return fPaths; }
			ShapeContainer*		Shapes() const
									{ return fShapes; }

 private:
			PathContainer*		fPaths;
			ShapeContainer*		fShapes;
};

#endif // ICON_H
