/*
 * PS.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 * Copyright 2010 Ithamar Adema.
 */


#include "PS.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <Path.h>

#include "DbgMsg.h"
#include "FilterIO.h"
#include "Halftone.h"
#include "JobData.h"
#include "PackBits.h"
#include "PPDParser.h"
#include "PrinterData.h"
#include "PSCap.h"
#include "PSData.h"
#include "UIDriver.h"
#include "ValidRect.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif


PSDriver::PSDriver(BMessage* msg, PrinterData* printer_data,
	const PrinterCap* printer_cap)
	:
	GraphicsDriver(msg, printer_data, printer_cap)
{
	fPrintedPages = 0;
	fHalftone = NULL;
	fFilterIO = NULL;
}


void
PSDriver::StartFilterIfNeeded()
{
	const PSData* data = dynamic_cast<const PSData*>(getPrinterData());
	PPDParser parser(BPath(data->fPPD.String()));
	if (parser.InitCheck() == B_OK) {
		BString param = parser.GetParameter("FoomaticRIPCommandLine");
		char str[3] = "%?";
		// for now, we don't have any extra substitutions to do...
		// (will be added once we support configuration options from the PPD)
		for (str[1] = 'A'; str[1] <= 'Z'; str[1]++)
			param.ReplaceAll(str, "");

		if (param.Length())
			fFilterIO = new FilterIO(param);

		if (!fFilterIO || fFilterIO->InitCheck() != B_OK) {
			delete fFilterIO;
			fFilterIO = NULL;
		}
	}
}


void
PSDriver::FlushFilterIfNeeded()
{
	if (fFilterIO) {
		char buffer[1024];
		ssize_t len;

		while ((len = fFilterIO->Read(buffer, sizeof(buffer))) > 0)
			writeSpoolData(buffer, len);
	}
}


void
PSDriver::writePSString(const char* format, ...)
{
	char str[256];
	va_list	ap;
	va_start(ap, format);
	vsprintf(str, format, ap);

	if (fFilterIO)
		fFilterIO->Write(str, strlen(str));
	else
		writeSpoolData(str, strlen(str));

	va_end(ap);
}


void 
PSDriver::writePSData(const void* data, size_t size)
{
	if (fFilterIO)
		fFilterIO->Write(data, size);
	else
		writeSpoolData(data, size);
}


bool
PSDriver::startDoc()
{
	try {
		StartFilterIfNeeded();

		jobStart();
		fHalftone = new Halftone(getJobData()->getSurfaceType(),
			getJobData()->getGamma(), getJobData()->getInkDensity(),
			getJobData()->getDitherType());
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
PSDriver::startPage(int page)
{
	page ++;
	writePSString("%%%%Page: %d %d\n", page, page);
	writePSString("gsave\n");
	setupCTM();
	return true;
}


bool
PSDriver::endPage(int)
{
	try {
		fPrintedPages ++;
		writePSString("grestore\n");
		writePSString("showpage\n");

		FlushFilterIfNeeded();

		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


void
PSDriver::setupCTM()
{
	const float leftMargin = getJobData()->getPrintableRect().left;
	const float topMargin = getJobData()->getPrintableRect().top;
	if (getJobData()->getOrientation() == JobData::kPortrait) {
		// move origin from bottom left to top left
		// and set margin
		writePSString("%f %f translate\n", leftMargin,
			getJobData()->getPaperRect().Height()-topMargin);
	} else {
		// landscape:
		// move origin from bottom left to margin top and left 
		// and rotate page contents
		writePSString("%f %f translate\n", topMargin, leftMargin);
		writePSString("90 rotate\n");
	}
	// y values increase from top to bottom
	// units of measure is dpi
	writePSString("72 %d div 72 -%d div scale\n", getJobData()->getXres(),
		getJobData()->getYres());
}


bool
PSDriver::endDoc(bool)
{
	try {
		if (fHalftone) {
			delete fHalftone;
		}
		jobEnd();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


inline uchar
hex_digit(uchar value) 
{
	if (value <= 9) return '0' + value;
	else return 'a' + (value - 10);
}


bool 
PSDriver::nextBand(BBitmap* bitmap, BPoint* offset)
{
	DBGMSG(("> nextBand\n"));

	try {
		BRect bounds = bitmap->Bounds();

		RECT rc;
		rc.left = (int)bounds.left;
		rc.top = (int)bounds.top;
		rc.right = (int)bounds.right;
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
			int widthByte = (width + 7) / 8;
				// byte boundary
			int height = rc.bottom - rc.top + 1;
			int in_size = color ? width : widthByte;
			int out_size = color ? width * 6: widthByte * 2;
			int delta = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->get_pixel_depth() = %d\n",
				fHalftone->getPixelDepth()));

			uchar* ptr = static_cast<uchar*>(bitmap->Bits())
						+ rc.top * delta
						+ (rc.left * fHalftone->getPixelDepth()) / 8;

			int compression_method;
			int compressed_size;
			const uchar* buffer;

			uchar* in_buffer = new uchar[in_size];
				// gray values
			uchar* out_buffer = new uchar[out_size];
				// gray values in hexadecimal

			auto_ptr<uchar> _in_buffer(in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			DBGMSG(("move\n"));

			int size = color ? width * 3 : in_size;
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

		} else
			DBGMSG(("band bitmap is clean.\n"));

		if (y >= page_height) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else
			offset->y += height;

		DBGMSG(("< nextBand\n"));
		return true;
	}
	catch (TransportException& err) {
		BAlert* alert = new BAlert("", err.what(), "OK");
		alert->Go();
		return false;
	} 
}


void 
PSDriver::jobStart()
{
	// PostScript header
	writePSString("%%!PS-Adobe-3.0\n");
	writePSString("%%%%LanguageLevel: 1\n");
	writePSString("%%%%Title: %s\n",
		getSpoolMetaData()->getDescription().c_str());
	writePSString("%%%%Creator: %s\n",
		getSpoolMetaData()->getMimeType().c_str());
	writePSString("%%%%CreationDate: %s",
		getSpoolMetaData()->getCreationTime().c_str());
	writePSString("%%%%DocumentMedia: Plain %d %d white 0 ( )\n",
		getJobData()->getPaperRect().IntegerWidth(),
		getJobData()->getPaperRect().IntegerHeight());
	writePSString("%%%%Pages: (atend)\n");	
	writePSString("%%%%EndComments\n");
	
	writePSString("%%%%BeginDefaults\n");
	writePSString("%%%%PageMedia: Plain\n");
	writePSString("%%%%EndDefaults\n");
}


void 
PSDriver::startRasterGraphics(int x, int y, int width, int height,
	int widthByte)
{
	bool color = getJobData()->getColor() == JobData::kColor;
	fCompressionMethod = -1;
	writePSString("gsave\n");
	writePSString("/s %d string def\n", widthByte);
	writePSString("%d %d translate\n", x, y);
	writePSString("%d %d scale\n", width, height);
	if (color)
		writePSString("%d %d 8\n", width, height); // 8 bpp
	else
		writePSString("%d %d 1\n", width, height); // 1 bpp
	writePSString("[%d 0 0 %d 0 0]\n", width, height);
	writePSString("{ currentfile s readhexstring pop }\n");
	if (color) {
		writePSString("false 3\n"); // single data source, 3 color components
		writePSString("colorimage\n");
	} else
		writePSString("image\n\n");
}


void 
PSDriver::endRasterGraphics()
{
	writePSString("grestore\n");
}


void 
PSDriver::rasterGraphics(
	int compression_method,
	const uchar* buffer,
	int size)
{
	if (fCompressionMethod != compression_method) {
		fCompressionMethod = compression_method;
	}
	writePSData(buffer, size);
	writePSString("\n");
}


void 
PSDriver::jobEnd()
{
	writePSString("%%%%Pages: %d\n", fPrintedPages);
	writePSString("%%%%EOF\n");

	FlushFilterIfNeeded();
}
