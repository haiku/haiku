/*
 * PCL5.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PCL6_H
#define __PCL6_H

#include "GraphicsDriver.h"
#include "jetlib.h"

class Halftone;

class PCL6Driver : public GraphicsDriver, private MJetlibClient {
public:
	PCL6Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap);

    virtual void FlushOutBuffer(HP_StreamHandleType pStream,
                                unsigned long cookie,
                                HP_pUByte pOutBuffer,
                                HP_SInt32 currentBufferLen);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page);
	virtual bool endDoc(bool success);

private:
	HP_UByte mediaSize(JobData::PAPER paper);
	void move(int x, int y);
	void jobStart();
	void startRasterGraphics(int x, int y, int width, int height);
	void endRasterGraphics();
	void rasterGraphics(
		int compression_method,
		const uchar *buffer,
		int size);
	void jobEnd();

	HP_StreamHandleType __stream;
	int __compression_method;
	Halftone *__halftone;
};

#endif	/* __PCL5_H */
