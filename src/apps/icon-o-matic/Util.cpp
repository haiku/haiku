/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Util.h"

#include <new>

#include "AddPathsCommand.h"
#include "AddStylesCommand.h"
#include "PathContainer.h"
#include "Style.h"
#include "StyleContainer.h"
#include "VectorPath.h"

using std::nothrow;

// new_style
void
new_style(rgb_color color, StyleContainer* container,
		  Style** style, AddStylesCommand** command)
{
	*style = new (nothrow) Style(color);
	if (*style) {
		Style* styles[1];
		styles[0] = *style;
		*command = new (nothrow) AddStylesCommand(
			container, styles, 1,
			container->CountStyles());
		if (*command == NULL) {
			delete *style;
			*style = NULL;
		}
	} else {
		*command = NULL;
	}
}

// new_path
void
new_path(PathContainer* container, VectorPath** path,
		 AddPathsCommand** command, VectorPath* other)
{
	if (other)
		*path = new (nothrow) VectorPath(*other);
	else
		*path = new (nothrow) VectorPath();

	if (*path) {
		VectorPath* paths[1];
		paths[0] = *path;
		int32 insertIndex = other ? container->IndexOf(other) + 1
								  : container->CountPaths();
		*command = new (nothrow) AddPathsCommand(
			container, paths, 1, true, insertIndex);
		if (*command == NULL) {
			delete *path;
			*path = NULL;
		}
	} else {
		*command = NULL;
	}
}




