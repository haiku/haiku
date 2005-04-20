// RenderingBuffer.h

#ifndef RENDERING_BUFFER_H
#define RENDERING_BUFFER_H

#include <GraphicsDefs.h>

class RenderingBuffer {
 public:
								RenderingBuffer() {}
	virtual						~RenderingBuffer() {}

	virtual	status_t			InitCheck() const = 0;

	virtual	color_space			ColorSpace() const = 0;
	virtual	void*				Bits() const = 0;
	virtual	uint32				BytesPerRow() const = 0;
	// the *count* of the pixels per line
	virtual	uint32				Width() const = 0;
	// the *count* of lines
	virtual	uint32				Height() const = 0;

	inline	uint32				BitsLength() const
									{ return Height() * BytesPerRow(); }
};

#endif // RENDERING_BUFFER_H
