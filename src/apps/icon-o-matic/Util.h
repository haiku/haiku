/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef UTIL_H
#define UTIL_H


#include <GraphicsDefs.h>

#include "IconBuild.h"


class AddPathsCommand;
class AddStylesCommand;

_BEGIN_ICON_NAMESPACE
	class PathContainer;
	class Style;
	class StyleContainer;
	class VectorPath;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


void new_style(rgb_color color, StyleContainer* container,
			   Style** style, AddStylesCommand** command);

void new_path(PathContainer* container, VectorPath** path,
			  AddPathsCommand** command, VectorPath* other = NULL);

#endif	// UTIL_H
