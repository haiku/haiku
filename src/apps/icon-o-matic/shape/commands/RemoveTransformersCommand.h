/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REMOVE_TRANSFORMERS_COMMAND_H
#define REMOVE_TRANSFORMERS_COMMAND_H


#include "IconBuild.h"
#include "RemoveCommand.h"


_BEGIN_ICON_NAMESPACE
	class Transformer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class RemoveTransformersCommand : public RemoveCommand<Transformer> {
 public:
								RemoveTransformersCommand(
									Container<Transformer>* container,
									const int32* indices,
									int32 count);
	virtual						~RemoveTransformersCommand();

	virtual void				GetName(BString& name);
};

#endif // REMOVE_TRANSFORMERS_COMMAND_H
