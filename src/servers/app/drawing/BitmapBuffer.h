// BitmapBuffer.h

#ifndef BITMAP_BUFFER_H
#define BITMAP_BUFFER_H

#include "RenderingBuffer.h"

class ServerBitmap;

class BitmapBuffer : public RenderingBuffer {
 public:
								BitmapBuffer(ServerBitmap* bitmap);
	virtual						~BitmapBuffer();

	virtual	status_t			InitCheck() const;

	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

								// BitmapBuffer
			const ServerBitmap*	Bitmap() const
									{ return fBitmap; }
 private:

			ServerBitmap*		fBitmap;
};

#endif // BITMAP_BUFFER_H
