/*
 * PCL5.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <memory>
#include "PCL5.h"
#include "UIDriver.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PCL5Cap.h"
#include "PackBits.h"
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

PCL5Driver::PCL5Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: GraphicsDriver(msg, printer_data, printer_cap)
{
	__halftone = NULL;
}

bool PCL5Driver::startDoc()
{
	try {
		jobStart();
		__halftone = new Halftone(getJobData()->getSurfaceType(), getJobData()->getGamma());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PCL5Driver::startPage(int)
{
	return true;
}

bool PCL5Driver::endPage(int)
{
	try {
		writeSpoolChar('\014');
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PCL5Driver::endDoc(bool)
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

bool PCL5Driver::nextBand(BBitmap *bitmap, BPoint *offset)
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
			int in_size   = widthByte;
			int out_size  = (widthByte * 6 + 4) / 5;
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

			DBGMSG(("move\n"));

			move(x, y);
			startRasterGraphics();

			for (int i = rc.top; i <= rc.bottom; i++) {
				__halftone->dither(in_buffer, ptr, x, y, width);
	
				compressed_size = pack_bits(out_buffer, in_buffer, in_size);
				
				if (compressed_size < in_size) {
					compression_method = 2; // back bits
					buffer = out_buffer;
				} else {
					compression_method = 0; // uncompressed
					buffer = in_buffer;
					compressed_size = in_size;
				}
	
				rasterGraphics(
					compression_method,
					buffer,
					compressed_size);

				ptr  += delta;
				y++;
			}

			endRasterGraphics();

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

void PCL5Driver::jobStart()
{
	// reset
	writeSpoolString("\033E");
	// dpi
	writeSpoolString("\033*t%dR", getJobData()->getXres());
	// unit of measure
	writeSpoolString("\033&u%dD", getJobData()->getXres());
	// page size
	writeSpoolString("\033&l0A");
	// page orientation
	writeSpoolString("\033&l0O");
	// raster presentation
	writeSpoolString("\033*r0F");
	// top maring and perforation skip
	writeSpoolString("\033&l0e0L");
	// clear horizontal margins
	writeSpoolString("\0339");
	// number of copies
	// writeSpoolString("\033&l%ldL", getJobData()->getCopies());
}

void PCL5Driver::startRasterGraphics()
{
	writeSpoolString("\033*r1A");
	__compression_method = -1;
}

void PCL5Driver::endRasterGraphics()
{
	writeSpoolString("\033*rB");
}

void PCL5Driver::rasterGraphics(
	int compression_method,
	const uchar *buffer,
	int size)
{
	if (__compression_method != compression_method) {
		writeSpoolString("\033*b%dM", compression_method);
		__compression_method = compression_method;
	}
	writeSpoolString("\033*b%dW", size);
	writeSpoolData(buffer, size);
}

void PCL5Driver::jobEnd()
{
	writeSpoolString("\033&l1T");
	writeSpoolString("\033E");
}

void PCL5Driver::move(int x, int y)
{
	writeSpoolString("\033*p%dx%dY", x, y+75);
}
