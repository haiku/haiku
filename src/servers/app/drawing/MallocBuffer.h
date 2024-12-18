// MallocBuffer.h

#ifndef MALLOC_BUFFER_H
#define MALLOC_BUFFER_H

#include "RenderingBuffer.h"

class BBitmap;

class MallocBuffer : public RenderingBuffer {
 public:
								MallocBuffer(uint32 width,
											 uint32 height);
	virtual						~MallocBuffer();

	virtual	status_t			InitCheck() const;

	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

 private:

			void*				fBuffer;
			uint32				fWidth;
			uint32				fHeight;
};

#endif // MALLOC_BUFFER_H
