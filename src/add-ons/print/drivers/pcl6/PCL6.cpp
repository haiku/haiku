/*
 * PCL6.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <memory>
#include "PCL6.h"
#include "UIDriver.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PCL6Cap.h"
#include "PackBits.h"
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

PCL6Driver::PCL6Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: GraphicsDriver(msg, printer_data, printer_cap)
{
	fHalftone = NULL;
	fStream = NULL;
}

void PCL6Driver::FlushOutBuffer(HP_StreamHandleType pStream, unsigned long cookie, HP_pUByte pOutBuffer, HP_SInt32 currentBufferLen) 
{
	writeSpoolData(pOutBuffer, currentBufferLen);
}


bool PCL6Driver::startDoc()
{
	try {
		jobStart();
		fHalftone = new Halftone(getJobData()->getSurfaceType(), getJobData()->getGamma(), getJobData()->getInkDensity(), getJobData()->getDitherType());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PCL6Driver::endDoc(bool)
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

bool PCL6Driver::nextBand(BBitmap *bitmap, BPoint *offset)
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

			bool color;
			int width;
			int widthByte;
			int padBytes;
			int out_row_length;
			int height;
			int out_size;
			int delta;

			color = getJobData()->getColor() == JobData::kColor;

			width = rc.right - rc.left + 1;
			height = rc.bottom - rc.top + 1;
			delta = bitmap->BytesPerRow();

			if (color) {
				widthByte = 3 * width;
			} else {
				widthByte = (width + 7) / 8;	/* byte boundary */
			}

			out_row_length = 4*((widthByte+3)/4);
			padBytes = out_row_length - widthByte; /* line length is a multiple of 4 bytes */
			out_size  = out_row_length * height;


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

			uchar *out_buffer = new uchar[out_size]; 

			uchar *out_ptr = out_buffer;

			auto_ptr<uchar> _out_buffer(out_buffer);

			DBGMSG(("move\n"));

			move(x, y);
			startRasterGraphics(x, y, width, height);

			compression_method = 0; // uncompressed
			buffer = out_buffer;
			compressed_size = out_size;

			// dither entire band into out_buffer
			for (int i = rc.top; i <= rc.bottom; i++) {
				uchar* out = out_ptr;
				if (color) {
					uchar* in = ptr;
					for (int w = width; w > 0; w --) {
						*out++ = in[2];
						*out++ = in[1];
						*out++ = in[0];
						in += 4;
					}
				} else {
					fHalftone->dither(out_ptr, ptr, x, y, width);
					// invert pixels
					for (int w = widthByte; w > 0; w --, out ++) {
						*out = ~*out;
					}
				}
				// pad with 0s
				for (int w = padBytes; w > 0; w --, out ++) {
					*out = 0;
				}

				ptr  += delta;
				out_ptr += out_row_length;
				y++;
			}

			rasterGraphics(
				compression_method,
				buffer,
				compressed_size);

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

void PCL6Driver::jobStart()
{
	// PJL header
	writeSpoolString("\033%%-12345X@PJL JOB\n"
					 "@PJL SET RESOLUTION=%d\n"
	                 "@PJL ENTER LANGUAGE=PCLXL\n"
	                 ") HP-PCL XL;1;1;"
	                 "Comment Copyright (c) 2003, 2004 Haiku\n",
	                 getJobData()->getXres());
	// PCL6 begin
	fStream = HP_NewStream(16 * 1024, this);
	HP_BeginSession_2(fStream, getJobData()->getXres(), getJobData()->getYres(), HP_eInch, HP_eBackChAndErrPage);
	HP_OpenDataSource_1(fStream, HP_eDefaultDataSource, HP_eBinaryLowByteFirst);	
}

bool PCL6Driver::startPage(int)
{
	HP_UByte orientation = HP_ePortraitOrientation;
	if (getJobData()->getOrientation() == JobData::kLandscape) {
		orientation = HP_eLandscapeOrientation;
	}
	HP_BeginPage_3(fStream, orientation, mediaSize(getJobData()->getPaper()), HP_eAutoSelect);
	// PageOrigin from Windows NT printer driver
	int x = 142 * getJobData()->getXres() / 600;
	int y = 100 * getJobData()->getYres() / 600;
	bool color = getJobData()->getColor() == JobData::kColor;
	HP_SetPageOrigin_1(fStream, x, y);
	HP_SetColorSpace_1(fStream, color ? HP_eRGB : HP_eGray);
	HP_SetPaintTxMode_1(fStream, HP_eOpaque);
	HP_SetSourceTxMode_1(fStream, HP_eOpaque);
	HP_SetROP_1(fStream, 204);
	return true;
}

void PCL6Driver::startRasterGraphics(int x, int y, int width, int height)
{
	bool color = getJobData()->getColor() == JobData::kColor;
	fCompressionMethod = -1;
	HP_BeginImage_1(fStream, HP_eDirectPixel, color ? HP_e8Bit : HP_e1Bit, width, height, width, height);
	HP_ReadImage_1(fStream, 0, height, HP_eNoCompression);
}

void PCL6Driver::endRasterGraphics()
{
	HP_EndImage_1(fStream);
}

void PCL6Driver::rasterGraphics(
	int compression_method,
	const uchar *buffer,
	int size)
{
	if (fCompressionMethod != compression_method) {
		fCompressionMethod = compression_method;
	}
	HP_EmbeddedDataPrefix32(fStream, size);
	HP_RawUByteArray(fStream, (uchar*)buffer, size);
}

bool PCL6Driver::endPage(int)
{
	try {
		HP_EndPage_2(fStream, getJobData()->getCopies());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

void PCL6Driver::jobEnd()
{
	HP_CloseDataSource_1(fStream);
	HP_EndSession_1(fStream);
	HP_FinishStream(fStream);
	// PJL footer
	writeSpoolString("\033%%-12345X@PJL EOJ\n"
	                 "\033%%-12345X");
}

void PCL6Driver::move(int x, int y)
{
	HP_SetCursor_1(fStream, x, y);
}

HP_UByte PCL6Driver::mediaSize(JobData::Paper paper)
{
	switch (paper) {
		case JobData::kLetter: return HP_eLetterPaper;
		case JobData::kLegal: return HP_eLegalPaper;
		case JobData::kA4: return HP_eA4Paper;
		case JobData::kExecutive: return HP_eExecPaper;
		case JobData::kLedger: return HP_eLedgerPaper;
		case JobData::kA3: return HP_eA3Paper;
/*
		case : return HP_eCOM10Envelope;
		case : return HP_eMonarchEnvelope;
		case : return HP_eC5Envelope;
		case : return HP_eDLEnvelope;
		case : return HP_eJB4Paper;
		case : return HP_eJB5Paper;
		case : return HP_eB5Envelope;
		case : return HP_eB5Paper;
		case : return HP_eJPostcard;
		case : return HP_eJDoublePostcard;
		case : return HP_eA5Paper;
		case : return HP_eA6Paper;
		case : return HP_eJB6Paper;
		case : return HP_eJIS8KPaper;
		case : return HP_eJIS16KPaper;
		case : return HP_eJISExecPaper;
*/
		default:
			return HP_eLegalPaper;
	}
}

