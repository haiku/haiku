/*
 * PS.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PS_H
#define __PS_H

#include "GraphicsDriver.h"

class Halftone;

class PSDriver : public GraphicsDriver {
public:
	PSDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page);
	virtual bool endDoc(bool success);

private:
	void setupCTM();
	void jobStart();
	void startRasterGraphics(int x, int y, int width, int height, int widthByte);
	void endRasterGraphics();
	void rasterGraphics(
		int compression_method,
		const uchar *buffer,
		int size);
	void jobEnd();

	int fPrintedPages;
	int fCompressionMethod;
	Halftone *fHalftone;
};

#endif	/* __PS_H */
