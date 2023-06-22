/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_PATHS_COMMAND_H
#define ADD_PATHS_COMMAND_H


#include "AddCommand.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class VectorPath;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddPathsCommand : public AddCommand<VectorPath> {
 public:
								AddPathsCommand(
									Container<VectorPath>* container,
									const VectorPath* const* paths,
									int32 count,
									bool ownsPaths,
									int32 index);
	virtual						~AddPathsCommand();

	virtual void				GetName(BString& name);
};

#endif // ADD_PATHS_COMMAND_H
