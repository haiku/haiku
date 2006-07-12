/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "IconObject.h"

class VertexSource {
 public:
								VertexSource();
	virtual						~VertexSource();

    virtual	void				rewind(unsigned path_id) = 0;
    virtual	unsigned			vertex(double* x, double* y) = 0;

	virtual	bool				WantsOpenPaths() const = 0;
	virtual	double				ApproximationScale() const = 0;
};


class Transformer : public VertexSource,
					public IconObject {
 public:
								Transformer(VertexSource& source,
											const char* name);
	virtual						~Transformer();

	virtual	Transformer*		Clone(VertexSource& source) const = 0;

	virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	bool				WantsOpenPaths() const;
	virtual	double				ApproximationScale() const;

 protected:
			VertexSource&		fSource;
};

#endif // TRANSFORMER_H
