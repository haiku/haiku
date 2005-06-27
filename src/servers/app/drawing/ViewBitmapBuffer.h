// ViewBitmapBuffer.h

#ifndef VIEW_BITMAP_BUFFER_H
#define VIEW_BITMAP_BUFFER_H

#include "RenderingBuffer.h"

class BBitmap;

class ViewBitmapBuffer : public RenderingBuffer {
 public:
								ViewBitmapBuffer(BBitmap* bitmap);
	virtual						~ViewBitmapBuffer();

	virtual	status_t			InitCheck() const;

	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

								// ViewBitmapBuffer
			const BBitmap*		Bitmap() const
									{ return fBitmap; }
 private:

			BBitmap*			fBitmap;
};

#endif // VIEW_BITMAP_BUFFER_H
