/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "Selectable.h"

class VertexSource {
 public:
								VertexSource();
	virtual						~VertexSource();

    virtual	void				rewind(unsigned path_id) = 0;
    virtual	unsigned			vertex(double* x, double* y) = 0;
};


class Transformer : public VertexSource,
					public Selectable {
 public:
								Transformer(VertexSource& source);
	virtual						~Transformer();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	const char*			Name() const = 0;

	// Selectable interface
	virtual	void				SelectedChanged();

 private:
			VertexSource&		fSource;
};

#endif // TRANSFORMER_H
