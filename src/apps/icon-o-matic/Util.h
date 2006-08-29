/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef UTIL_H
#define UTIL_H

#include <GraphicsDefs.h>

class AddPathsCommand;
class AddStylesCommand;
class PathContainer;
class Style;
class StyleContainer;
class VectorPath;

void new_style(rgb_color color, StyleContainer* container,
			   Style** style, AddStylesCommand** command);

void new_path(PathContainer* container, VectorPath** path,
			  AddPathsCommand** command, VectorPath* other = NULL);


#endif // UTIL_H
