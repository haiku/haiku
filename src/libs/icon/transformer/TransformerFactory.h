/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORMER_FACTORY_H
#define TRANSFORMER_FACTORY_H


#include <String.h>

#include "IconBuild.h"


class BMessage;


_BEGIN_ICON_NAMESPACE


class Transformer;
class VertexSource;

class TransformerFactory {
 public:

	static	Transformer*		TransformerFor(uint32 type,
											   VertexSource& source);

	static	Transformer*		TransformerFor(BMessage* archive,
											   VertexSource& source);

#ifdef ICON_O_MATIC
	static	bool				NextType(int32* cookie,
										 uint32* type,
										 BString* name);

#endif // ICON_O_MATIC
};


_END_ICON_NAMESPACE


#endif	// TRANSFORMER_FACTORY_H
