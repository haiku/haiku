/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef MOVE_TRANSFORMERS_COMMAND_H
#define MOVE_TRANSFORMERS_COMMAND_H


#include "IconBuild.h"
#include "MoveCommand.h"


_BEGIN_ICON_NAMESPACE
	class Transformer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MoveTransformersCommand : public MoveCommand<Transformer> {
 public:
								MoveTransformersCommand(
									Container<Transformer>* shape,
									Transformer** transformers,
									int32 count,
									int32 toIndex);
	virtual						~MoveTransformersCommand();

	virtual void				GetName(BString& name);
};

#endif // MOVE_TRANSFORMERS_COMMAND_H
