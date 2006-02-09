/*
 * PCL6.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#ifndef __PCL6_H
#define __PCL6_H


#include "GraphicsDriver.h"
#include "PCL6Cap.h"
#include "PCL6Writer.h"

class Halftone;

class PCL6Driver : public GraphicsDriver, public PCL6WriterStream 
{
public:
	PCL6Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap);

	void write(const uint8 *data, uint32 size);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page);
	virtual bool endDoc(bool success);

private:
	bool supportsDeltaRowCompression();
	PCL6Writer::MediaSize mediaSize(JobData::Paper paper);
	PCL6Writer::MediaSource mediaSource(JobData::PaperSource source);
	void move(int x, int y);
	void jobStart();
	void writeBitmap(const uchar* buffer, int outSize, int rowSize, int x, int y, int width, int height, int deltaRowSize);
	void startRasterGraphics(int x, int y, int width, int height, PCL6Writer::Compression compressionMethod);
	void endRasterGraphics();
	void rasterGraphics(
		const uchar *buffer,
		int bufferSize,
		int dataSize,
		int rowSize,
		int height,
		int compression_method);
	void jobEnd();

	PCL6Writer            *fWriter;
	PCL6Writer::MediaSide fMediaSide; // side if in duplex mode
	Halftone              *fHalftone;
};

#endif	/* __PCL6_H */
