/*
 * PCL5.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PCL5_H
#define __PCL5_H

#include "GraphicsDriver.h"

class Halftone;

class PCL5Driver : public GraphicsDriver {
public:
	PCL5Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page);
	virtual bool endDoc(bool success);

private:
	void move(int x, int y);
	void jobStart();
	void startRasterGraphics(int width, int height);
	void endRasterGraphics();
	void rasterGraphics(
		int compression_method,
		const uchar *buffer,
		int size,
		bool lastPlane);
	void jobEnd();
	int bytesToEnterCompressionMethod(int compression_method);

	int fCompressionMethod;
	Halftone *fHalftone;
};

#endif	/* __PCL5_H */
