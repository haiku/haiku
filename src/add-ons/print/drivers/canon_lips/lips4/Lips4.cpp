/*
 * Lips4.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips4Cap.h"

#include <memory>

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>

#include "DbgMsg.h"
#include "Halftone.h"
#include "JobData.h"
#include "Lips4.h"
#include "PackBits.h"
#include "PrinterData.h"
#include "ValidRect.h"


LIPS4Driver::LIPS4Driver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap),
	fHalftone(NULL)
{
}


bool
LIPS4Driver::StartDocument()
{
	try {
		_BeginTextMode();
		_JobStart();
		_ColorModeDeclaration();
		_SoftReset();
		_SizeUnitMode();
		_SelectSizeUnit();
		_PaperFeedMode();
		_SelectPageFormat();
		_DisableAutoFF();
		_SetNumberOfCopies();
		_SidePrintingControl();
		_SetBindingMargin();
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
LIPS4Driver::StartPage(int)
{
	try {
		fCurrentX = 0;
		fCurrentY = 0;
		_MemorizedPosition();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
LIPS4Driver::EndPage(int)
{
	try {
		_FormFeed();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
LIPS4Driver::EndDocument(bool)
{
	try {
		if (fHalftone)
			delete fHalftone;

		_JobEnd();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
LIPS4Driver::NextBand(BBitmap* bitmap, BPoint* offset)
{
	DBGMSG(("> nextBand\n"));

	try {

		if (bitmap == NULL) {
			uchar dummy[1];
			dummy[0] =  '\0';
			_RasterGraphics(1, 1, 1, 0, dummy);
			DBGMSG(("< next_band\n"));
			return true;
		}

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

		if (y + height > page_height)
			height = page_height - y;

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
			int height = rc.bottom - rc.top + 1;
			int in_size = widthByte * height;
			int out_size = (in_size * 6 + 4) / 5;
			int delta = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("in_size = %d\n", in_size));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("fHalftone_engine->Get_pixel_depth() = %d\n",
				fHalftone->GetPixelDepth()));

			uchar* ptr = static_cast<uchar*>(bitmap->Bits())
						+ rc.top * delta
						+ (rc.left * fHalftone->GetPixelDepth()) / 8;

			int compression_method;
			int compressed_size;
			const uchar* buffer;

			uchar* in_buffer  = new uchar[in_size];
			uchar* out_buffer = new uchar[out_size];

			auto_ptr<uchar> _in_buffer (in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			uchar* ptr2 = static_cast<uchar *>(in_buffer);

			DBGMSG(("move\n"));

			_Move(x, y);

			for (int i = rc.top; i <= rc.bottom; i++) {
				fHalftone->Dither(ptr2, ptr, x, y, width);
				ptr  += delta;
				ptr2 += widthByte;
				y++;
			}

			DBGMSG(("PackBits\n"));

			compressed_size = pack_bits(out_buffer, in_buffer, in_size);

			if (compressed_size < in_size) {
				compression_method = 11;
				buffer = out_buffer;
			} else if (compressed_size > out_size) {
				BAlert* alert = new BAlert("memory overrun!!!", "warning", "OK");
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
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

			_RasterGraphics(
				compressed_size,	// size,
				widthByte,			// widthByte
				height,				// height,
				compression_method,
				buffer);

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
LIPS4Driver::_BeginTextMode()
{
	WriteSpoolString("\033%%@");
}


void
LIPS4Driver::_JobStart()
{
	WriteSpoolString("\033P41;%d;1J\033\\", GetJobData()->GetXres());
}


void
LIPS4Driver::_ColorModeDeclaration()
{
//	if (color)
//		WriteSpoolString("\033[1\"p");
//	else
		WriteSpoolString("\033[0\"p");
}


void
LIPS4Driver::_SoftReset()
{
	WriteSpoolString("\033<");
}


void
LIPS4Driver::_SizeUnitMode()
{
	WriteSpoolString("\033[11h");
}


void
LIPS4Driver::_SelectSizeUnit()
{
	WriteSpoolString("\033[?7;%d I", GetJobData()->GetXres());
}


void
LIPS4Driver::_PaperFeedMode()
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

	switch (GetJobData()->GetPaperSource()) {
		case JobData::kManual:
			i = 10;
			break;
		case JobData::kUpper:
			i = 11;
			break;
		case JobData::kMiddle:
			i = 12;
			break;
		case JobData::kLower:
			i = 13;
			break;
		case JobData::kAuto:
		default:
			i = 0;
			break;
	}

	WriteSpoolString("\033[%dq", i);
}


void
LIPS4Driver::_SelectPageFormat()
{
	int i;

	switch (GetJobData()->GetPaper()) {
		case JobData::kA3:
			i = 12;
			break;

		case JobData::kA4:
			i = 14;
			break;

		case JobData::kA5:
			i = 16;
			break;

		case JobData::kJapanesePostcard:
			i = 18;
			break;

		case JobData::kB4:
			i = 24;
			break;

		case JobData::kB5:
			i = 26;
			break;

		case JobData::kLetter:
			i = 30;
			break;

		case JobData::kLegal:
			i = 32;
			break;

		case JobData::kExecutive:
			i = 40;
			break;

		case JobData::kJEnvYou4:
			i = 50;
			break;

		case JobData::kUser:
			i = 90;
			break;

		default:
			i = 0;
			break;
	}

	if (JobData::kLandscape == GetJobData()->GetOrientation())
		i++;

	WriteSpoolString("\033[%d;;p", i);
}


void
LIPS4Driver::_DisableAutoFF()
{
	WriteSpoolString("\033[?2h");
}


void
LIPS4Driver::_SetNumberOfCopies()
{
	WriteSpoolString("\033[%ldv", GetJobData()->GetCopies());
}


void
LIPS4Driver::_SidePrintingControl()
{
	if (GetJobData()->GetPrintStyle() == JobData::kSimplex)
		WriteSpoolString("\033[0#x");
	else
		WriteSpoolString("\033[2;0#x");
}


void
LIPS4Driver::_SetBindingMargin()
{
	if (GetJobData()->GetPrintStyle() == JobData::kDuplex) {
		int i;
//		switch (job_data()->binding_location()) {
//		case kLongEdgeLeft:
			i = 0;
//			break;
//		case kLongEdgeRight:
//			i = 1;
//			break;
//		case kShortEdgeTop:
//			i = 2;
//			break;
//		case kShortEdgeBottom:
//			i = 3;
//			break;
//		}
		WriteSpoolString("\033[%d;0#w", i);
	}
}


void
LIPS4Driver::_MemorizedPosition()
{
	WriteSpoolString("\033[0;1;0x");
}


void
LIPS4Driver::_MoveAbsoluteHorizontal(int x)
{
	WriteSpoolString("\033[%ld`", x);
}


void
LIPS4Driver::_CarriageReturn()
{
	WriteSpoolChar('\x0d');
}


void
LIPS4Driver::_MoveDown(int dy)
{
	WriteSpoolString("\033[%lde", dy);
}


void
LIPS4Driver::_RasterGraphics(int compression_size, int widthbyte, int height,
	int compression_method, const uchar* buffer)
{
// 0  RAW
// 10 RLE
// 11 packbits

	WriteSpoolString(
		"\033[%ld;%ld;%d;%ld;%ld.r",
		compression_size,
		widthbyte,
		GetJobData()->GetXres(),
		compression_method,
		height);

	WriteSpoolData(buffer, compression_size);
}


void
LIPS4Driver::_FormFeed()
{
	WriteSpoolChar('\014');
}


void
LIPS4Driver::_JobEnd()
{
	WriteSpoolString("\033P0J\033\\");
}


void
LIPS4Driver::_Move(int x, int y)
{
	if (fCurrentX != x) {
		if (x) {
			_MoveAbsoluteHorizontal(x);
		} else {
			_CarriageReturn();
		}
		fCurrentX = x;
	}
	if (fCurrentY != y) {
		int dy = y - fCurrentY;
		_MoveDown(dy);
		fCurrentY = y;
	}
}
