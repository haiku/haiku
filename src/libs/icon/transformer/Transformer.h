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


_BEGIN_ICON_NAMESPACE


class VertexSource {
 public:
								VertexSource();
	virtual						~VertexSource();

    virtual	void				rewind(unsigned path_id) = 0;
    virtual	unsigned			vertex(double* x, double* y) = 0;

	virtual	bool				WantsOpenPaths() const = 0;
	virtual	double				ApproximationScale() const = 0;
};


#ifdef ICON_O_MATIC
class Transformer : public VertexSource,
					public IconObject {
#else
class Transformer : public VertexSource {
#endif
 public:
								Transformer(VertexSource& source,
											const char* name);
								Transformer(VertexSource& source,
											BMessage* archive);

	virtual						~Transformer();

	// Transformer
	virtual	Transformer*		Clone(VertexSource& source) const = 0;

	virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	bool				WantsOpenPaths() const;
	virtual	double				ApproximationScale() const;

 protected:
			VertexSource&		fSource;
};


_END_ICON_NAMESPACE


#endif	// TRANSFORMER_H
