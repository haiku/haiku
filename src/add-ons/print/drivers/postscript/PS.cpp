/*
 * PS.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <memory>
#include <stdio.h>
#include "PS.h"
#include "UIDriver.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PSCap.h"
#include "PackBits.h"
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

PSDriver::PSDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: GraphicsDriver(msg, printer_data, printer_cap)
{
	fPrintedPages = 0;
	fHalftone = NULL;
}

bool PSDriver::startDoc()
{
	try {
		jobStart();
		fHalftone = new Halftone(getJobData()->getSurfaceType(), getJobData()->getGamma());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PSDriver::startPage(int page)
{
	page ++;
	writeSpoolString("%%%%Page: %d %d\n", page, page);
	return true;
}

bool PSDriver::endPage(int)
{
	try {
		fPrintedPages ++;
		writeSpoolString("showpage\n");
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PSDriver::endDoc(bool)
{
	try {
		if (fHalftone) {
			delete fHalftone;
		}
		jobEnd();
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

inline uchar hex_digit(uchar value) 
{
	if (value <= 9) return '0'+value;
	else return 'a'+(value-10);
}

bool PSDriver::nextBand(BBitmap *bitmap, BPoint *offset)
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

		if (get_valid_rect(bitmap, &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;

			bool color = getJobData()->getColor() == JobData::kColor;
			int width = rc.right - rc.left + 1;
			int widthByte = (width + 7) / 8;	/* byte boundary */
			int height    = rc.bottom - rc.top + 1;
			int in_size   = color ? width : widthByte;
			int out_size  = color ? width * 6: widthByte * 2;
			int delta     = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->get_pixel_depth() = %d\n", fHalftone->getPixelDepth()));

			uchar *ptr = (uchar *)bitmap->Bits()
						+ rc.top * delta
						+ (rc.left * fHalftone->getPixelDepth()) / 8;

			int compression_method;
			int compressed_size;
			const uchar *buffer;

			uchar *in_buffer = new uchar[in_size]; // gray values
			uchar *out_buffer = new uchar[out_size]; // gray values in hexadecimal

			auto_ptr<uchar> _in_buffer(in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			DBGMSG(("move\n"));

			int size = color ? width*3 : in_size;
			startRasterGraphics(x, y, width, height, size);

			for (int i = rc.top; i <= rc.bottom; i++) {
				if (color) {
					uchar* out = out_buffer;
					uchar* in  = ptr;
					for (int w = width; w > 0; w --) {
						*out++ = hex_digit((in[2]) >> 4);
						*out++ = hex_digit((in[2]) & 15);
						*out++ = hex_digit((in[1]) >> 4);
						*out++ = hex_digit((in[1]) & 15);
						*out++ = hex_digit((in[0]) >> 4);
						*out++ = hex_digit((in[0]) & 15);
						in += 4;
					}
				} else {
					fHalftone->dither(in_buffer, ptr, x, y, width);

					uchar* in = in_buffer;
					uchar* out = out_buffer;
				
					for (int w = in_size; w > 0; w --, in ++) {
						*in = ~*in; // invert pixels
						*out++ = hex_digit((*in) >> 4);
						*out++ = hex_digit((*in) & 15);
					}
				}
				
				{	
					compression_method = 0; // uncompressed
					buffer = out_buffer;
					compressed_size = out_size;
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

void PSDriver::jobStart()
{
	// PostScript header
	writeSpoolString("%%!PS-Adobe-3.0\n");
	writeSpoolString("%%%%LanguageLevel: 1\n");
	writeSpoolString("%%%%Title: %s\n", getSpoolMetaData()->getDescription().c_str());	
	writeSpoolString("%%%%Creator: %s\n", getSpoolMetaData()->getMimeType().c_str());
	writeSpoolString("%%%%CreationDate: %s", getSpoolMetaData()->getCreationTime().c_str());
	writeSpoolString("%%%%DocumentMedia: Plain %d %d white 0 ( )\n", getJobData()->getPaperRect().IntegerWidth(), getJobData()->getPaperRect().IntegerHeight());
	writeSpoolString("%%%%Pages: (atend)\n");	
	writeSpoolString("%%%%EndComments\n");
	
	writeSpoolString("%%%%BeginDefaults\n");
	writeSpoolString("%%%%PageMedia: Plain\n");
	writeSpoolString("%%%%EndDefaults\n");
	
	// setup CTM
	writeSpoolString("%%%%BeginSetup\n");
	// move origin from bottom left to top left
	writeSpoolString("0 %f translate\n", getJobData()->getPaperRect().Height());
	// y values increase from top to bottom
	// units of measure is dpi
	writeSpoolString("72 %d div 72 -%d div scale\n", getJobData()->getXres(), getJobData()->getYres());
	writeSpoolString("%%%%EndSetup\n");
}

void PSDriver::startRasterGraphics(int x, int y, int width, int height, int widthByte)
{
	bool color = getJobData()->getColor() == JobData::kColor;
	fCompressionMethod = -1;
	writeSpoolString("gsave\n");
	writeSpoolString("/s %d string def\n", widthByte);
	writeSpoolString("%d %d translate\n", x, y);
	writeSpoolString("%d %d scale\n", width, height);
	if (color) {
		writeSpoolString("%d %d 8\n", width, height); // 8 bpp
	} else {
		writeSpoolString("%d %d 1\n", width, height); // 1 bpp
	}
	writeSpoolString("[%d 0 0 %d 0 0]\n", width, height);
	writeSpoolString("{ currentfile s readhexstring pop }\n");
	if (color) {
		writeSpoolString("false 3\n"); // single data source, 3 color components
		writeSpoolString("colorimage\n");
	} else {
		writeSpoolString("image\n\n");
	}
}

void PSDriver::endRasterGraphics()
{
	writeSpoolString("grestore\n");
}

void PSDriver::rasterGraphics(
	int compression_method,
	const uchar *buffer,
	int size)
{
	if (fCompressionMethod != compression_method) {
		fCompressionMethod = compression_method;
	}
	writeSpoolData(buffer, size);
	writeSpoolString("\n");
}

void PSDriver::jobEnd()
{
	writeSpoolString("%%%%Pages: %d\n", fPrintedPages);
	writeSpoolString("%%%%EOF\n");
}
