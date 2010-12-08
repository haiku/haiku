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
						PSDriver(BMessage* message, PrinterData* printerData,
							const PrinterCap* printerCap);

protected:
	virtual	bool		StartDocument();
	virtual	bool		StartPage(int page);
	virtual	bool		NextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool		EndPage(int page);
	virtual	bool		EndDocument(bool success);

private:
			void		_SetupCTM();
			void		_JobStart();
			void		_StartRasterGraphics(int x, int y, int width,
							int height, int widthByte);
			void		_EndRasterGraphics();
			void		_RasterGraphics(int compression_method,
							const uchar* buffer, int size);
			void		_JobEnd();

			void		_StartFilterIfNeeded();
			void		_FlushFilterIfNeeded();
			void		_WritePSString(const char* format, ...);
			void		_WritePSData(const void* data, size_t size);

			int			fPrintedPages;
			int			fCompressionMethod;
			Halftone*	fHalftone;
			FilterIO*	fFilterIO;
};

#endif // __PS_H
