// RenderingBuffer.h

#ifndef RENDERING_BUFFER_H
#define RENDERING_BUFFER_H

#include <GraphicsDefs.h>

class RenderingBuffer {
 public:
								RenderingBuffer() {};
	virtual						~RenderingBuffer() {};

	virtual	status_t			InitCheck() const = 0;

	virtual	color_space			ColorSpace() const = 0;
	virtual	void*				Bits() const = 0;
	virtual	uint32				BytesPerRow() const = 0;
	virtual	uint32				Width() const = 0;
	virtual	uint32				Height() const = 0;
};

#endif // RENDERING_BUFFER_H
