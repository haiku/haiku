#ifndef D_WINDOW_BUFFER_H
#define D_WINDOW_BUFFER_H

#include "RenderingBuffer.h"

struct direct_buffer_info;

class DWindowBuffer : public RenderingBuffer {
 public:
								DWindowBuffer();
	virtual						~DWindowBuffer();
	
	virtual	status_t			InitCheck() const;
	
	virtual	color_space			ColorSpace() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;
	
			void				SetTo(direct_buffer_info* info);

			BRegion&			WindowClipping()
									{ return fWindowClipping; }
 private:
			uint8*				fBits;			
			uint32				fWidth;
			uint32				fHeight;
			uint32				fBytesPerRow;
			color_space			fFormat;

			BRegion				fWindowClipping;
};

#endif // D_WINDOW_BUFFER_H
