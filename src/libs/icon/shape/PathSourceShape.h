/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PATH_SOURCE_SHAPE_H
#define PATH_SOURCE_SHAPE_H


#include "IconBuild.h"
#include "Shape.h"


class BMessage;


_BEGIN_ICON_NAMESPACE


class PathContainer;
class PropertyObject;
class Style;


class PathSourceShape : public _ICON_NAMESPACE Shape {
 public:
	enum {
		archive_code = 'shps'
	};

									PathSourceShape(_ICON_NAMESPACE Style* style);
									PathSourceShape(const PathSourceShape& other);
	virtual							~PathSourceShape();

	// IconObject interface
	virtual	status_t				Unarchive(BMessage* archive);
#ifdef ICON_O_MATIC
	virtual	status_t				Archive(BMessage* into,
											bool deep = true) const;

	virtual	PropertyObject*			MakePropertyObject() const;
	virtual	bool					SetToPropertyObject(
										const PropertyObject* object);
#else
	inline	void					Notify() {}
#endif // ICON_O_MATIC

	// PathSourceShape
	virtual	status_t				InitCheck() const;
	virtual PathSourceShape*		Clone() const
										{ return new PathSourceShape(*this); }

			void					SetStyle(_ICON_NAMESPACE Style* style)
										{ Shape::SetStyle(style); }
	inline	_ICON_NAMESPACE Style*	Style() const
										{ return Shape::Style(); }

			void					SetMinVisibilityScale(float scale);
			float					MinVisibilityScale() const
										{ return fMinVisibilityScale; }
			void					SetMaxVisibilityScale(float scale);
			float					MaxVisibilityScale() const
										{ return fMaxVisibilityScale; }

	virtual bool					Visible(float scale) const;

 private:
			float					fMinVisibilityScale;
			float					fMaxVisibilityScale;
};


_END_ICON_NAMESPACE


#endif	// PATH_SOURCE_SHAPE_H
