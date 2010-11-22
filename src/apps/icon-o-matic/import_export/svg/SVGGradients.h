/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SVG_GRADIENTS_H
#define SVG_GRADIENTS_H


#include <agg_color_rgba.h>
#include <agg_trans_affine.h>

#include <Message.h>
#include <String.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Gradient;
_END_ICON_NAMESPACE

namespace agg {
namespace svg {

class SVGGradient : public BMessage {
 public:
							SVGGradient();
	virtual					~SVGGradient();

			void			SetID(const char* id);
			const char*		ID() const;

	virtual	void			AddStop(float offset, rgba8 color);
			void			SetTransformation(const trans_affine& transform);

			Gradient*		GetGradient(BRect objectBounds);

 protected:
	virtual	Gradient*		MakeGradient() const = 0;
			void			IdentifyGradientUnits();

 private:

	enum {
		UNSPECIFIED = 0,
		USER_SPACE_ON_USE,
		OBJECT_BOUNDING_BOX,
	};

			Gradient*		fGradient;
			BString			fID;
			uint32			fGradientUnits;
};

class SVGLinearGradient : public SVGGradient {
 public:
							SVGLinearGradient();
	virtual					~SVGLinearGradient();

 protected:
	virtual	Gradient*		MakeGradient() const;

};

class SVGRadialGradient : public SVGGradient {
 public:
							SVGRadialGradient();
	virtual					~SVGRadialGradient();

 protected:
	virtual	Gradient*		MakeGradient() const;

};

} // namespace svg
} // namespace agg

#endif // SVG_GRADIENTS_H
