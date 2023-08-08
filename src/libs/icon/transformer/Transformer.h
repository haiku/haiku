/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORMER_H
#define TRANSFORMER_H


#ifdef ICON_O_MATIC
#	include "IconObject.h"
#else
#	include <Message.h>
#	include <SupportDefs.h>
#endif

#include "IconBuild.h"
#include "VertexSource.h"


_BEGIN_ICON_NAMESPACE


/*! Base class for all transformers.
	All child classes should inherit either PathTransformer, StyleTransformer,
	or both.
*/
#ifdef ICON_O_MATIC
class Transformer : public IconObject {
#else
	class Transformer {
#endif
public:
#ifdef ICON_O_MATIC
								Transformer(const char* name)
									: IconObject(name) {}
								Transformer(BMessage* archive)
									: IconObject(archive) {}
#else
								Transformer(const char* name) {}
								Transformer(BMessage* archive) {}
#endif

	virtual						~Transformer() {}

	virtual	Transformer*		Clone() const = 0;
};


_END_ICON_NAMESPACE


#endif	// TRANSFORMER_H
