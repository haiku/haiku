/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#ifndef DOCUMENT_BUILD_H
#define DOCUMENT_BUILD_H

#include <stdio.h>

#include <Rect.h>
#include <String.h>

#include "IconBuild.h"
#include "nanosvg.h"


class SVGImporter;

_BEGIN_ICON_NAMESPACE
	class Icon;
	class Transformable;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class DocumentBuilder {
 public:

								DocumentBuilder(NSVGimage* image);

			void				SetTitle(const char* title);
			void				SetDimensions(uint32 width, uint32 height, BRect viewBox);


			status_t			GetIcon(Icon* icon,
										SVGImporter* importer,
										const char* fallbackName);

 private:
			status_t			_AddShape(NSVGshape* svgShape,
										  bool outline,
										  const Transformable& transform,
										  Icon* icon);


			uint32				fWidth;
			uint32				fHeight;
			BRect				fViewBox;
			BString				fTitle;

			NSVGimage*			fSource;
};

#endif // DOCUMENT_BUILD_H
