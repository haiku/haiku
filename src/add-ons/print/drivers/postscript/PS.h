/*
 * PS.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */
#ifndef __PS_H
#define __PS_H


#include "GraphicsDriver.h"


class FilterIO;
class Halftone;


class PSDriver : public GraphicsDriver {
public:
						PSDriver(BMessage* msg, PrinterData* printer_data,
							const PrinterCap* printer_cap);

protected:
	virtual	bool		startDoc();
	virtual	bool		startPage(int page);
	virtual	bool		nextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool		endPage(int page);
	virtual	bool		endDoc(bool success);

private:
			void		setupCTM();
			void		jobStart();
			void		startRasterGraphics(int x, int y, int width,
							int height, int widthByte);
			void		endRasterGraphics();
			void		rasterGraphics(int compression_method,
							const uchar* buffer, int size);
			void		jobEnd();

			void		StartFilterIfNeeded();
			void		FlushFilterIfNeeded();
			void		writePSString(const char* format, ...);
			void		writePSData(const void* data, size_t size);

			int			fPrintedPages;
			int			fCompressionMethod;
			Halftone*	fHalftone;
			FilterIO*	fFilterIO;
};

#endif // __PS_H
