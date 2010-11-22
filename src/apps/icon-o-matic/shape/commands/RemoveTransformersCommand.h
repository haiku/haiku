/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef REMOVE_TRANSFORMERS_COMMAND_H
#define REMOVE_TRANSFORMERS_COMMAND_H


#include "Command.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Shape;
	class Transformer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


// TODO: make a templated "remove items" command?

class RemoveTransformersCommand : public Command {
 public:
								RemoveTransformersCommand(
									Shape* container,
									const int32* indices,
									int32 count);
	virtual						~RemoveTransformersCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Shape*				fContainer;
			Transformer**		fTransformers;
			int32*				fIndices;
			int32				fCount;
			bool				fTransformersRemoved;
};

#endif // REMOVE_TRANSFORMERS_COMMAND_H
