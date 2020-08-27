// BBitmapBuffer.h

#ifndef B_BITMAP_BUFFER_H
#define B_BITMAP_BUFFER_H

#include "RenderingBuffer.h"

#include <AutoDeleter.h>

class BBitmap;

class BBitmapBuffer : public RenderingBuffer {
 public:
								BBitmapBuffer(BBitmap* bitmap);
	virtual						~BBitmapBuffer();

	virtual	status_t			InitCheck() const;

	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

								// BBitmapBuffer
			const BBitmap*		Bitmap() const
									{ return fBitmap.Get(); }
 private:

			ObjectDeleter<BBitmap>
								fBitmap;
};

#endif // B_BITMAP_BUFFER_H
