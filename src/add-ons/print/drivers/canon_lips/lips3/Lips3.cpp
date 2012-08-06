/*
 * Lips3.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips3.h"

#include <memory>

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>

#include "Compress3.h"
#include "DbgMsg.h"
#include "Halftone.h"
#include "JobData.h"
#include "Lips3Cap.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "ValidRect.h"


LIPS3Driver::LIPS3Driver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap),
	fHalftone(NULL)
{
}


bool
LIPS3Driver::StartDocument()
{
	try {
		_BeginTextMode();
		_JobStart();
		_SoftReset();
		_SizeUnitMode();
		_SelectSizeUnit();
		_SelectPageFormat();
		_PaperFeedMode();
		_DisableAutoFF();
		_SetNumberOfCopies();
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
LIPS3Driver::StartPage(int)
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
LIPS3Driver::EndPage(int)
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
LIPS3Driver::EndDocument(bool)
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
LIPS3Driver::NextBand(BBitmap* bitmap, BPoint* offset)
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
				// byte boundary
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
			DBGMSG(("renderobj->Get_pixel_depth() = %d\n",
				fHalftone->GetPixelDepth()));

			uchar* ptr = static_cast<uchar*>(bitmap->Bits())
						+ rc.top * delta
						+ (rc.left * fHalftone->GetPixelDepth()) / 8;

			int compressionMethod;
			int compressedSize;
			const uchar* buffer;

			uchar* in_buffer = new uchar[in_size];
			uchar* out_buffer = new uchar[out_size];

			auto_ptr<uchar> _in_buffer (in_buffer);
			auto_ptr<uchar> _out_buffer(out_buffer);

			uchar* ptr2 = static_cast<uchar*>(in_buffer);

			DBGMSG(("move\n"));

			_Move(x, y);

			for (int i = rc.top; i <= rc.bottom; i++) {
				fHalftone->Dither(ptr2, ptr, x, y, width);
				ptr  += delta;
				ptr2 += widthByte;
				y++;
			}

			compressedSize = compress3(out_buffer, in_buffer, in_size);

			if (compressedSize < in_size) {
				compressionMethod = 9;
					// compress3
				buffer = out_buffer;
			} else if (compressedSize > out_size) {
				BAlert* alert = new BAlert("memory overrun!!!", "warning",
					"OK");
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
				return false;
			} else {
				compressionMethod = 0;
				buffer = in_buffer;
				compressedSize = in_size;
			}

			DBGMSG(("compressedSize = %d\n", compressedSize));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("compression_method = %d\n", compressionMethod));

			_RasterGraphics(
				compressedSize,	// size,
				widthByte,			// widthByte
				height,				// height,
				compressionMethod,
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
LIPS3Driver::_BeginTextMode()
{
	WriteSpoolString("\033%%@");
}


void
LIPS3Driver::_JobStart()
{
	WriteSpoolString("\033P31;300;1J\033\\");
}


void
LIPS3Driver::_SoftReset()
{
	WriteSpoolString("\033<");
}


void
LIPS3Driver::_SizeUnitMode()
{
	WriteSpoolString("\033[11h");
}


void
LIPS3Driver::_SelectSizeUnit()
{
	WriteSpoolString("\033[7 I");
}


void
LIPS3Driver::_SelectPageFormat()
{
	int i;
	int width  = 0;
	int height = 0;

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

//	case JobData::kExecutive:
//		i = 40;
//		break;
//
//	case JobData::kJEnvYou4:
//		i = 50;
//		break;
//
//	case JobData::kUser:
//		i = 90;
//		break;
//
	default:
		i = 80;
		width  = GetJobData()->GetPaperRect().IntegerWidth();
		height = GetJobData()->GetPaperRect().IntegerHeight();
		break;
	}

	if (JobData::kLandscape == GetJobData()->GetOrientation())
		i++;

	if (i < 80)
		WriteSpoolString("\033[%d;;p", i);
	else
		WriteSpoolString("\033[%d;%d;%dp", i, height, width);
}


void
LIPS3Driver::_PaperFeedMode()
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
			i = 1;
			break;
		case JobData::kLower:
			i = 2;
			break;
		case JobData::kUpper:
			i = 3;
			break;
		case JobData::kAuto:
		default:
			i = 0;
			break;
	}

	WriteSpoolString("\033[%dq", i);
}


void
LIPS3Driver::_DisableAutoFF()
{
	WriteSpoolString("\033[?2h");
}


void
LIPS3Driver::_SetNumberOfCopies()
{
	WriteSpoolString("\033[%ldv", GetJobData()->GetCopies());
}


void
LIPS3Driver::_MemorizedPosition()
{
	WriteSpoolString("\033[0;1;0x");
}


void
LIPS3Driver::_MoveAbsoluteHorizontal(int x)
{
	WriteSpoolString("\033[%d`", x);
}


void
LIPS3Driver::_CarriageReturn()
{
	WriteSpoolChar('\x0d');
}


void
LIPS3Driver::_MoveDown(int dy)
{
	WriteSpoolString("\033[%de", dy);
}


void
LIPS3Driver::_RasterGraphics( int compressionSize, int widthbyte, int height,
	int compressionMethod,	const uchar* buffer)
{
//  0 RAW
//  9 compress-3
	WriteSpoolString("\033[%d;%d;%d;%d;%d.r", compressionSize, widthbyte,
		GetJobData()->GetXres(), compressionMethod, height);

	WriteSpoolData(buffer, compressionSize);
}


void
LIPS3Driver::_FormFeed()
{
	WriteSpoolChar('\014');
}


void
LIPS3Driver::_JobEnd()
{
	WriteSpoolString("\033P0J\033\\");
}


void
LIPS3Driver::_Move(int x, int y)
{
	if (fCurrentX != x) {
		if (x)
			_MoveAbsoluteHorizontal(x);
		else
			_CarriageReturn();

		fCurrentX = x;
	}
	if (fCurrentY != y) {
		int dy = y - fCurrentY;
		_MoveDown(dy);
		fCurrentY = y;
	}
}
