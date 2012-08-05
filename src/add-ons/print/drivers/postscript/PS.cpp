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


PSDriver::PSDriver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap),
	fPrintedPages(0),
	fCompressionMethod(0),
	fHalftone(NULL),
	fFilterIO(NULL)
{
}


void
PSDriver::_StartFilterIfNeeded()
{
	const PSData* data = dynamic_cast<const PSData*>(GetPrinterData());
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
PSDriver::_FlushFilterIfNeeded()
{
	if (fFilterIO) {
		char buffer[1024];
		ssize_t len;

		while ((len = fFilterIO->Read(buffer, sizeof(buffer))) > 0)
			WriteSpoolData(buffer, len);
	}
}


void
PSDriver::_WritePSString(const char* format, ...)
{
	char str[256];
	va_list	ap;
	va_start(ap, format);
	vsprintf(str, format, ap);

	if (fFilterIO)
		fFilterIO->Write(str, strlen(str));
	else
		WriteSpoolData(str, strlen(str));

	va_end(ap);
}


void 
PSDriver::_WritePSData(const void* data, size_t size)
{
	if (fFilterIO)
		fFilterIO->Write(data, size);
	else
		WriteSpoolData(data, size);
}


bool
PSDriver::StartDocument()
{
	try {
		_StartFilterIfNeeded();

		_JobStart();
		fHalftone = new Halftone(GetJobData()->GetSurfaceType(),
			GetJobData()->GetGamma(), GetJobData()->GetInkDensity(),
			GetJobData()->GetDitherType());
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
PSDriver::StartPage(int page)
{
	page ++;
	_WritePSString("%%%%Page: %d %d\n", page, page);
	_WritePSString("gsave\n");
	_SetupCTM();
	return true;
}


bool
PSDriver::EndPage(int)
{
	try {
		fPrintedPages ++;
		_WritePSString("grestore\n");
		_WritePSString("showpage\n");

		_FlushFilterIfNeeded();

		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


void
PSDriver::_SetupCTM()
{
	const float leftMargin = GetJobData()->GetPrintableRect().left;
	const float topMargin = GetJobData()->GetPrintableRect().top;
	if (GetJobData()->GetOrientation() == JobData::kPortrait) {
		// move origin from bottom left to top left
		// and set margin
		_WritePSString("%f %f translate\n", leftMargin,
			GetJobData()->GetPaperRect().Height()-topMargin);
	} else {
		// landscape:
		// move origin from bottom left to margin top and left 
		// and rotate page contents
		_WritePSString("%f %f translate\n", topMargin, leftMargin);
		_WritePSString("90 rotate\n");
	}
	// y values increase from top to bottom
	// units of measure is dpi
	_WritePSString("72 %d div 72 -%d div scale\n", GetJobData()->GetXres(),
		GetJobData()->GetYres());
}


bool
PSDriver::EndDocument(bool)
{
	try {
		if (fHalftone) {
			delete fHalftone;
			fHalftone = NULL;
		}
		_JobEnd();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


static inline uchar
ToHexDigit(uchar value)
{
	if (value <= 9) return '0' + value;
	else return 'a' + (value - 10);
}


bool 
PSDriver::NextBand(BBitmap* bitmap, BPoint* offset)
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

		int page_height = GetPageHeight();

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

			bool color = GetJobData()->GetColor() == JobData::kColor;
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
			DBGMSG(("renderobj->Get_pixel_depth() = %d\n",
				fHalftone->GetPixelDepth()));

			uchar* ptr = static_cast<uchar*>(bitmap->Bits())
						+ rc.top * delta
						+ (rc.left * fHalftone->GetPixelDepth()) / 8;

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
			_StartRasterGraphics(x, y, width, height, size);

			for (int i = rc.top; i <= rc.bottom; i++) {
				if (color) {
					uchar* out = out_buffer;
					uchar* in  = ptr;
					for (int w = width; w > 0; w --) {
						*out++ = ToHexDigit((in[2]) >> 4);
						*out++ = ToHexDigit((in[2]) & 15);
						*out++ = ToHexDigit((in[1]) >> 4);
						*out++ = ToHexDigit((in[1]) & 15);
						*out++ = ToHexDigit((in[0]) >> 4);
						*out++ = ToHexDigit((in[0]) & 15);
						in += 4;
					}
				} else {
					fHalftone->Dither(in_buffer, ptr, x, y, width);

					uchar* in = in_buffer;
					uchar* out = out_buffer;
				
					for (int w = in_size; w > 0; w --, in ++) {
						*in = ~*in; // invert pixels
						*out++ = ToHexDigit((*in) >> 4);
						*out++ = ToHexDigit((*in) & 15);
					}
				}
				
				{	
					compression_method = 0; // uncompressed
					buffer = out_buffer;
					compressed_size = out_size;
				}

				_RasterGraphics(
					compression_method,
					buffer,
					compressed_size);

				ptr  += delta;
				y++;
			}

			_EndRasterGraphics();

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
		BAlert* alert = new BAlert("", err.What(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return false;
	} 
}


void 
PSDriver::_JobStart()
{
	// PostScript header
	_WritePSString("%%!PS-Adobe-3.0\n");
	_WritePSString("%%%%LanguageLevel: 1\n");
	_WritePSString("%%%%Title: %s\n",
		GetSpoolMetaData()->GetDescription().c_str());
	_WritePSString("%%%%Creator: %s\n",
		GetSpoolMetaData()->GetMimeType().c_str());
	_WritePSString("%%%%CreationDate: %s",
		GetSpoolMetaData()->GetCreationTime().c_str());
	_WritePSString("%%%%DocumentMedia: Plain %d %d white 0 ( )\n",
		GetJobData()->GetPaperRect().IntegerWidth(),
		GetJobData()->GetPaperRect().IntegerHeight());
	_WritePSString("%%%%Pages: (atend)\n");
	_WritePSString("%%%%EndComments\n");
	
	_WritePSString("%%%%BeginDefaults\n");
	_WritePSString("%%%%PageMedia: Plain\n");
	_WritePSString("%%%%EndDefaults\n");
}


void 
PSDriver::_StartRasterGraphics(int x, int y, int width, int height,
	int widthByte)
{
	bool color = GetJobData()->GetColor() == JobData::kColor;
	fCompressionMethod = -1;
	_WritePSString("gsave\n");
	_WritePSString("/s %d string def\n", widthByte);
	_WritePSString("%d %d translate\n", x, y);
	_WritePSString("%d %d scale\n", width, height);
	if (color)
		_WritePSString("%d %d 8\n", width, height); // 8 bpp
	else
		_WritePSString("%d %d 1\n", width, height); // 1 bpp
	_WritePSString("[%d 0 0 %d 0 0]\n", width, height);
	_WritePSString("{ currentfile s readhexstring pop }\n");
	if (color) {
		_WritePSString("false 3\n"); // single data source, 3 color components
		_WritePSString("colorimage\n");
	} else
		_WritePSString("image\n\n");
}


void 
PSDriver::_EndRasterGraphics()
{
	_WritePSString("grestore\n");
}


void 
PSDriver::_RasterGraphics(int compression_method, const uchar* buffer,
	int size)
{
	if (fCompressionMethod != compression_method) {
		fCompressionMethod = compression_method;
	}
	_WritePSData(buffer, size);
	_WritePSString("\n");
}


void 
PSDriver::_JobEnd()
{
	_WritePSString("%%%%Pages: %d\n", fPrintedPages);
	_WritePSString("%%%%EOF\n");

	_FlushFilterIfNeeded();
}
