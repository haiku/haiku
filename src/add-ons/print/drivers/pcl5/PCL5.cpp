/*
 * PCL5.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */


#include "PCL5.h"

#include <memory>

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>

#include "DbgMsg.h"
#include "Halftone.h"
#include "JobData.h"
#include "PackBits.h"
#include "PCL5Cap.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "ValidRect.h"


PCL5Driver::PCL5Driver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap),
	fCompressionMethod(0),
	fHalftone(NULL)
{
}


bool
PCL5Driver::StartDocument()
{
	try {
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
PCL5Driver::StartPage(int)
{
	return true;
}


bool
PCL5Driver::EndPage(int)
{
	try {
		WriteSpoolChar('\014');
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
PCL5Driver::EndDocument(bool)
{
	try {
		if (fHalftone != NULL) {
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


bool
PCL5Driver::NextBand(BBitmap* bitmap, BPoint* offset)
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

		int pageHeight = GetPageHeight();

		if (y + height > pageHeight)
			height = pageHeight - y;

		rc.bottom = height - 1;

		DBGMSG(("height = %d\n", height));
		DBGMSG(("x = %d\n", x));
		DBGMSG(("y = %d\n", y));

		if (get_valid_rect(bitmap, &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;

			int width = rc.right - rc.left + 1;
			int widthByte = (width + 7) / 8;
				// byte boundary
			int height = rc.bottom - rc.top + 1;
			int in_size = widthByte;
			int out_size = (widthByte * 6 + 4) / 5;
			int delta = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("in_size = %d\n", in_size));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->Get_pixel_depth() = %d\n", fHalftone->GetPixelDepth()));

			uchar* ptr = static_cast<uchar*>(bitmap->Bits())
						+ rc.top * delta
						+ (rc.left * fHalftone->GetPixelDepth()) / 8;

			int compressionMethod;
			int compressedSize;
			const uchar* buffer;

			uchar* in_buffer  = new uchar[in_size];
			uchar* out_buffer = new uchar[out_size];

			auto_ptr<uchar> _in_buffer (in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			DBGMSG(("move\n"));

			_Move(x, y);
			_StartRasterGraphics(width, height);

			const bool color = GetJobData()->GetColor() == JobData::kColor;
			const int num_planes = color ? 3 : 1;
			
			if (color) {
				fHalftone->SetPlanes(Halftone::kPlaneRGB1);
				fHalftone->SetBlackValue(Halftone::kLowValueMeansBlack);
			}
			
			for (int i = rc.top; i <= rc.bottom; i++) {
			
				for (int plane = 0; plane < num_planes; plane ++) {
										
					fHalftone->Dither(in_buffer, ptr, x, y, width);
							
					compressedSize = pack_bits(out_buffer, in_buffer, in_size);
					
					if (compressedSize + _BytesToEnterCompressionMethod(2)
						< in_size + _BytesToEnterCompressionMethod(0)) {
						compressionMethod = 2; // back bits
						buffer = out_buffer;
					} else {
						compressionMethod = 0; // uncompressed
						buffer = in_buffer;
						compressedSize = in_size;
					}
		
					_RasterGraphics(
						compressionMethod,
						buffer,
						compressedSize,
						plane == num_planes - 1);
				
				}

				ptr  += delta;
				y++;
			}

			_EndRasterGraphics();

		} else
			DBGMSG(("band bitmap is clean.\n"));

		if (y >= pageHeight) {
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
PCL5Driver::_JobStart()
{
	const bool color = GetJobData()->GetColor() == JobData::kColor;
	// enter PCL5
	WriteSpoolString("\033%%-12345X@PJL ENTER LANGUAGE=PCL\n");
	// reset
	WriteSpoolString("\033E");
	// dpi
	WriteSpoolString("\033*t%dR", GetJobData()->GetXres());
	// unit of measure
	WriteSpoolString("\033&u%dD", GetJobData()->GetXres());
	// page size
	WriteSpoolString("\033&l0A");
	// page orientation
	WriteSpoolString("\033&l0O");
	if (color) {
		// 3 color planes (red, green, blue)
		WriteSpoolString("\033*r3U");
	}
	// raster presentation
	WriteSpoolString("\033*r0F");
	// top maring and perforation skip
	WriteSpoolString("\033&l0e0L");
	// clear horizontal margins
	WriteSpoolString("\0339");
	// number of copies
	// WriteSpoolString("\033&l%ldL", GetJobData()->GetCopies());
}


void
PCL5Driver::_StartRasterGraphics(int width, int height)
{
	// width
	WriteSpoolString("\033*r%dS", width);
	// height
	WriteSpoolString("\033*r%dT", height);
	// start raster graphics
	WriteSpoolString("\033*r1A");
	fCompressionMethod = -1;
}


void
PCL5Driver::_EndRasterGraphics()
{
	WriteSpoolString("\033*rB");
}


void
PCL5Driver::_RasterGraphics(int compressionMethod, const uchar* buffer,
	int size, bool lastPlane)
{
	if (fCompressionMethod != compressionMethod) {
		WriteSpoolString("\033*b%dM", compressionMethod);
		fCompressionMethod = compressionMethod;
	}
	WriteSpoolString("\033*b%d", size);
	if (lastPlane)
		WriteSpoolString("W");
	else
		WriteSpoolString("V");

	WriteSpoolData(buffer, size);
}


void
PCL5Driver::_JobEnd()
{
	WriteSpoolString("\033&l1T");
	WriteSpoolString("\033E");
}


void
PCL5Driver::_Move(int x, int y)
{
	WriteSpoolString("\033*p%dx%dY", x, y + 75);
}


int
PCL5Driver::_BytesToEnterCompressionMethod(int compressionMethod)
{
	if (fCompressionMethod == compressionMethod)
		return 0;
	else
		return 5;
}
