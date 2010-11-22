/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ADD_PATHS_COMMAND_H
#define ADD_PATHS_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class VectorPath;
	class PathContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddPathsCommand : public Command {
 public:
								AddPathsCommand(
									PathContainer* container,
									VectorPath** const paths,
									int32 count,
									bool ownsPaths,
									int32 index);
	virtual						~AddPathsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PathContainer*		fContainer;
			VectorPath**		fPaths;
			int32				fCount;
			bool				fOwnsPaths;
			int32				fIndex;
			bool				fPathsAdded;
};

#endif // ADD_PATHS_COMMAND_H
