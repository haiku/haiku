/*
 * Lips3.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <memory>
#include "Lips3.h"
#include "UIDriver.h"
#include "Compress3.h"
#include "JobData.h"
#include "PrinterData.h"
#include "Lips3Cap.h"
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

LIPS3Driver::LIPS3Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: GraphicsDriver(msg, printer_data, printer_cap)
{
	__halftone = NULL;
}

bool LIPS3Driver::startDoc()
{
	try {
		beginTextMode();
		jobStart();
		softReset();
		sizeUnitMode();
		selectSizeUnit();
		selectPageFormat();
		paperFeedMode();
		disableAutoFF();
		setNumberOfCopies();
		__halftone = new Halftone(getJobData()->getSurfaceType(), getJobData()->getGamma());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool LIPS3Driver::startPage(int)
{
	try {
		__current_x = 0;
		__current_y = 0;
		memorizedPosition();
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool LIPS3Driver::endPage(int)
{
	try {
		formFeed();
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool LIPS3Driver::endDoc(bool)
{
	try {
		if (__halftone) {
			delete __halftone;
		}
		jobEnd();
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool LIPS3Driver::nextBand(BBitmap *bitmap, BPoint *offset)
{
	DBGMSG(("> nextBand\n"));

	try {
		BRect bounds = bitmap->Bounds();

		RECT rc;
		rc.left   = (int)bounds.left;
		rc.top    = (int)bounds.top;
		rc.right  = (int)bounds.right;
		rc.bottom = (int)bounds.bottom;

		int height = rc.bottom - rc.top + 1;

		int x = (int)offset->x;
		int y = (int)offset->y;

		int page_height = getPageHeight();

		if (y + height > page_height) {
			height = page_height - y;
		}

		rc.bottom = height - 1;

		DBGMSG(("height = %d\n", height));
		DBGMSG(("x = %d\n", x));
		DBGMSG(("y = %d\n", y));

		if (get_valid_rect(bitmap, __halftone->getPalette(), &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;

			int width = rc.right - rc.left + 1;
			int widthByte = (width + 7) / 8;	/* byte boundary */
			int height    = rc.bottom - rc.top + 1;
			int in_size   = widthByte * height;
			int out_size  = (in_size * 6 + 4) / 5;
			int delta     = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("in_size = %d\n", in_size));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->get_pixel_depth() = %d\n", __halftone->getPixelDepth()));

			uchar *ptr = (uchar *)bitmap->Bits()
						+ rc.top * delta
						+ (rc.left * __halftone->getPixelDepth()) / 8;

			int compression_method;
			int compressed_size;
			const uchar *buffer;

			uchar *in_buffer  = new uchar[in_size];
			uchar *out_buffer = new uchar[out_size];

			auto_ptr<uchar> _in_buffer (in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			uchar *ptr2 = (uchar *)in_buffer;

			DBGMSG(("move\n"));

			move(x, y);

			for (int i = rc.top; i <= rc.bottom; i++) {
				__halftone->dither(ptr2, ptr, x, y, width);
				ptr  += delta;
				ptr2 += widthByte;
				y++;
			}

			compressed_size = compress3(out_buffer, in_buffer, in_size);

			if (compressed_size < in_size) {
				compression_method = 9;		/* compress3 */
				buffer = out_buffer;
			} else if (compressed_size > out_size) {
				BAlert *alert = new BAlert("memory overrun!!!", "warning", "OK");
				alert->Go();
				return false;
			} else {
				compression_method = 0;
				buffer = in_buffer;
				compressed_size = in_size;
			}

			DBGMSG(("compressed_size = %d\n", compressed_size));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("compression_method = %d\n", compression_method));

			rasterGraphics(
				compressed_size,	// size,
				widthByte,			// widthByte
				height,				// height,
				compression_method,
				buffer);

		} else {
			DBGMSG(("band bitmap is clean.\n"));
		}

		if (y >= page_height) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else {
			offset->y += height;
		}

		DBGMSG(("< nextBand\n"));
		return true;
	}
	catch (TransportException &err) {
		BAlert *alert = new BAlert("", err.what(), "OK");
		alert->Go();
		return false;
	} 
}

void LIPS3Driver::beginTextMode()
{
	writeSpoolString("\033%%@");
}

void LIPS3Driver::jobStart()
{
	writeSpoolString("\033P31;300;1J\033\\");
}

void LIPS3Driver::softReset()
{
	writeSpoolString("\033<");
}

void LIPS3Driver::sizeUnitMode()
{
	writeSpoolString("\033[11h");
}

void LIPS3Driver::selectSizeUnit()
{
	writeSpoolString("\033[7 I");
}

void LIPS3Driver::selectPageFormat()
{
	int i;
	int width  = 0;
	int height = 0;

	switch (getJobData()->getPaper()) {
	case JobData::A3:
		i = 12;
		break;

	case JobData::A4:
		i = 14;
		break;

	case JobData::A5:
		i = 16;
		break;

	case JobData::JAPANESE_POSTCARD:
		i = 18;
		break;

	case JobData::B4:
		i = 24;
		break;

	case JobData::B5:
		i = 26;
		break;

	case JobData::LETTER:
		i = 30;
		break;

	case JobData::LEGAL:
		i = 32;
		break;

//	case JobData::EXECUTIVE:
//		i = 40;
//		break;
//
//	case JobData::JENV_YOU4:
//		i = 50;
//		break;
//
//	case JobData::USER:
//		i = 90;
//		break;
//
	default:
		i = 80;
		width  = getJobData()->getPaperRect().IntegerWidth();
		height = getJobData()->getPaperRect().IntegerHeight();
		break;
	}

	if (JobData::LANDSCAPE == getJobData()->getOrientation())
		i++;

	if (i < 80) {
		writeSpoolString("\033[%d;;p", i);
	} else {
		writeSpoolString("\033[%d;%d;%dp", i, height, width);
	}
}

void LIPS3Driver::paperFeedMode()
{
	// 0 auto
	// --------------
	// 1 MP tray
	// 2 lower
	// 3 lupper
	// --------------
	// 10 MP tray
	// 11 casette 1
	// 12 casette 2
	// 13 casette 3
	// 14 casette 4
	// 15 casette 5
	// 16 casette 6
	// 17 casette 7

	int i;

	switch (getJobData()->getPaperSource()) {
	case JobData::MANUAL:
		i = 1;
		break;
	case JobData::LOWER:
		i = 2;
		break;
	case JobData::UPPER:
		i = 3;
		break;
	case JobData::AUTO:
	default:
		i = 0;
		break;
	}

	writeSpoolString("\033[%dq", i);
}

void LIPS3Driver::disableAutoFF()
{
	writeSpoolString("\033[?2h");
}

void LIPS3Driver::setNumberOfCopies()
{
	writeSpoolString("\033[%ldv", getJobData()->getCopies());
}

void LIPS3Driver::memorizedPosition()
{
	writeSpoolString("\033[0;1;0x");
}

void LIPS3Driver::moveAbsoluteHorizontal(int x)
{
	writeSpoolString("\033[%d`", x);
}

void LIPS3Driver::carriageReturn()
{
	writeSpoolChar('\x0d');
}

void LIPS3Driver::moveDown(int dy)
{
	writeSpoolString("\033[%de", dy);
}

void LIPS3Driver::rasterGraphics(
	int compression_size,
	int widthbyte,
	int height,
	int compression_method,
	const uchar *buffer)
{
//  0 RAW
//  9 compress-3
	writeSpoolString(
		"\033[%d;%d;%d;%d;%d.r",
		compression_size,
		widthbyte,
		getJobData()->getXres(),
		compression_method,
		height);

	writeSpoolData(buffer, compression_size);
}

void LIPS3Driver::formFeed()
{
	writeSpoolChar('\014');
}

void LIPS3Driver::jobEnd()
{
	writeSpoolString("\033P0J\033\\");
}

void LIPS3Driver::move(int x, int y)
{
	if (__current_x != x) {
		if (x) {
			moveAbsoluteHorizontal(x);
		} else {
			carriageReturn();
		}
		__current_x = x;
	}
	if (__current_y != y) {
		int dy = y - __current_y;
		moveDown(dy);
		__current_y = y;
	}
}
