/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REMOVE_PATHS_COMMAND_H
#define REMOVE_PATHS_COMMAND_H


#include "Command.h"
#include "IconBuild.h"

#include <List.h>


_BEGIN_ICON_NAMESPACE
	class VectorPath;
	class PathContainer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemovePathsCommand : public Command {
 public:
								RemovePathsCommand(
									PathContainer* container,
									VectorPath** const paths,
									int32 count);
	virtual						~RemovePathsCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PathContainer*		fContainer;
			struct PathInfo {
				VectorPath*		path;
				int32			index;
				BList			shapes;
			};
			PathInfo*			fInfos;
			int32				fCount;
			bool				fPathsRemoved;
};

#endif // REMOVE_PATHS_COMMAND_H
