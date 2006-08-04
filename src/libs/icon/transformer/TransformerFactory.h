/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORMER_FACTORY_H
#define TRANSFORMER_FACTORY_H

#include <String.h>

class BMessage;
class Transformer;
class VertexSource;

class TransformerFactory {
 public:
								
	static	Transformer*		TransformerFor(uint32 type,
											   VertexSource& source);

#ifdef ICON_O_MATIC
	static	Transformer*		TransformerFor(BMessage* archive,
											   VertexSource& source);

	static	bool				NextType(int32* cookie,
										 uint32* type,
										 BString* name);

#endif // ICON_O_MATIC
};

#endif // TRANSFORMER_FACTORY_H
