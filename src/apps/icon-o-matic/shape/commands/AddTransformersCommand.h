/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ADD_TRANSFORMERS_COMMAND_H
#define ADD_TRANSFORMERS_COMMAND_H


#include "AddCommand.h"
#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Transformer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class AddTransformersCommand : public AddCommand<Transformer> {
 public:
								AddTransformersCommand(
									Container<Transformer>* container,
									const Transformer* const* transformers,
									int32 count,
									int32 index);
	virtual						~AddTransformersCommand();

	virtual void				GetName(BString& name);
};

#endif // ADD_TRANSFORMERS_COMMAND_H
