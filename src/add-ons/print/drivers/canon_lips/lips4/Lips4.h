/*
 * Lips4.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __LIPS4_H
#define __LIPS4_H

#include "GraphicsDriver.h"

class Halftone;

class LIPS4Driver : public GraphicsDriver {
public:
	LIPS4Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *cap);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page);
	virtual bool endDoc(bool success);

private:
	void move(int x, int y);
	void beginTextMode();
	void jobStart();
	void colorModeDeclaration();
	void softReset();
	void sizeUnitMode();
	void selectSizeUnit();
	void paperFeedMode();
	void selectPageFormat();
	void disableAutoFF();
	void setNumberOfCopies();
	void sidePrintingControl();
	void setBindingMargin();
	void memorizedPosition();
	void moveAbsoluteHorizontal(int x);
	void carriageReturn();
	void moveDown(int dy);
	void rasterGraphics(
		int size,
		int widthbyte,
		int height,
		int compression_method,
		const uchar *buffer);
	void formFeed();
	void jobEnd();

	int __current_x;
	int __current_y;
	Halftone *__halftone;
};

#endif	/* __LIPS4_H */
