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
					PCL6Driver(BMessage* message, PrinterData* printerData,
						const PrinterCap* printerCap);

			void	Write(const uint8* data, uint32 size);

protected:
	virtual	bool	StartDocument();
	virtual	bool	StartPage(int page);
	virtual	bool	NextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool	EndPage(int page);
	virtual	bool	EndDocument(bool success);

private:
			bool	_SupportsRLECompression();
			bool	_SupportsDeltaRowCompression();
			bool	_UseColorMode();
			PCL6Writer::MediaSize _MediaSize(JobData::Paper paper);
			PCL6Writer::MediaSource	_MediaSource(JobData::PaperSource source);
			void	_Move(int x, int y);
			void	_JobStart();
			void	_WriteBitmap(const uchar* buffer, int outSize, int rowSize,
						int x, int y, int width, int height, int deltaRowSize);
			void	_StartRasterGraphics(int x, int y, int width, int height,
						PCL6Writer::Compression compressionMethod);
			void	_EndRasterGraphics();
			void	_RasterGraphics(const uchar* buffer, int bufferSize,
						int dataSize, int rowSize, int height,
						int compression_method);
			void	_JobEnd();

			PCL6Writer*	fWriter;
			PCL6Writer::MediaSide fMediaSide; // side if in duplex mode
			Halftone*	fHalftone;
};

#endif // __PCL6_H
