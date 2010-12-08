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
						PCL5Driver(BMessage* message, PrinterData* printerData,
							const PrinterCap* printerCap);

protected:
	virtual	bool		StartDocument();
	virtual	bool		StartPage(int page);
	virtual	bool		NextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool		EndPage(int page);
	virtual	bool		EndDocument(bool success);

private:
			void		_Move(int x, int y);
			void		_JobStart();
			void		_StartRasterGraphics(int width, int height);
			void		_EndRasterGraphics();
			void		_RasterGraphics(int compression_method,
							const uchar* buffer, int size, bool lastPlane);
			void		_JobEnd();
			int			_BytesToEnterCompressionMethod(int compression_method);

			int			fCompressionMethod;
			Halftone*	fHalftone;
};

#endif // __PCL5_H
