/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPJob.h"

#include <Debug.h>


GPJob::GPJob()
	:
	fApplicationName(),
	fOutputStream(NULL),
	fConfiguration(NULL),
	fFirstPage(true),
	fVariables(NULL),
	fBands(NULL),
	fCachedBand(NULL),
	fWriteError(B_OK)
{
	fImage.init = ImageInit;
	fImage.reset = ImageReset;
	fImage.width = ImageWidth;
	fImage.height = ImageHeight;
	fImage.get_row = ImageGetRow;
	fImage.get_appname = ImageGetAppname;
	fImage.conclude = ImageConclude;
	fImage.rep = this;
}


GPJob::~GPJob()
{
}


void
GPJob::SetApplicationName(const BString& applicationName)
{
	fApplicationName = applicationName;
}


void
GPJob::SetConfiguration(GPJobConfiguration* configuration)
{
	fConfiguration = configuration;
}


void
GPJob::SetOutputStream(OutputStream* outputStream)
{
	fOutputStream = outputStream;
}


status_t
GPJob::Begin()
{
	fWriteError = B_OK;

	stp_init();

	fPrinter = stp_get_printer_by_driver(fConfiguration->fDriver);
	if (fPrinter == NULL) {
		fprintf(stderr, "GPJob Begin: driver %s not found!\n",
			fConfiguration->fDriver.String());
		return B_ERROR;
	}

	fVariables = stp_vars_create();
	if (fVariables == NULL) {
		fprintf(stderr, "GPJob Begin: out of memory\n");
		return B_NO_MEMORY;
	}
	stp_set_printer_defaults(fVariables, fPrinter);

	stp_set_outfunc(fVariables, OutputFunction);
	stp_set_errfunc(fVariables, ErrorFunction);
	stp_set_outdata(fVariables, this);
	stp_set_errdata(fVariables, this);

	stp_set_string_parameter(fVariables, "PageSize",
		fConfiguration->fPageSize);

	if (fConfiguration->fResolution != "")
		stp_set_string_parameter(fVariables, "Resolution",
			fConfiguration->fResolution);

	stp_set_string_parameter(fVariables, "InputSlot",
		fConfiguration->fInputSlot);

	stp_set_string_parameter(fVariables, "PrintingMode",
		fConfiguration->fPrintingMode);

	{
		map<string, string>::iterator it = fConfiguration->fStringSettings.
			begin();
		for (; it != fConfiguration->fStringSettings.end(); it ++) {
			stp_set_string_parameter(fVariables, it->first.c_str(),
				it->second.c_str());
		}
	}

	{
		map<string, bool>::iterator it = fConfiguration->fBooleanSettings.
			begin();
		for (; it != fConfiguration->fBooleanSettings.end(); it ++) {
			stp_set_boolean_parameter(fVariables, it->first.c_str(),
				it->second);
		}
	}

	{
		map<string, int32>::iterator it = fConfiguration->fIntSettings.
			begin();
		for (; it != fConfiguration->fIntSettings.end(); it ++) {
			stp_set_int_parameter(fVariables, it->first.c_str(),
				it->second);
		}
	}

	{
		map<string, int32>::iterator it = fConfiguration->fDimensionSettings.
			begin();
		for (; it != fConfiguration->fDimensionSettings.end(); it ++) {
			stp_set_dimension_parameter(fVariables, it->first.c_str(),
				it->second);
		}
	}

	{
		map<string, double>::iterator it = fConfiguration->fDoubleSettings.
			begin();
		for (; it != fConfiguration->fDoubleSettings.end(); it ++) {
			stp_set_float_parameter(fVariables, it->first.c_str(),
				it->second);
		}
	}

	stp_set_string_parameter(fVariables, "InputImageType",
		"RGB");
	stp_set_string_parameter(fVariables, "ChannelBitDepth",
		"8");
	stp_set_float_parameter(fVariables, "Density",
		1.0f);
	stp_set_string_parameter(fVariables, "JobMode", "Job");

	stp_set_printer_defaults_soft(fVariables, fPrinter);

	return B_OK;
}


void
GPJob::End()
{
	if (fVariables == NULL)
		return;

	if (!fFirstPage)
		stp_end_job(fVariables, &fImage);

	stp_vars_destroy(fVariables);
	fVariables = NULL;
}


static int
gcd(int a, int b)
{
	// Euclidean algorithm for greatest common divisor
	while (b != 0) {
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}


static int
to72dpiFloor(int value, int fromUnit) {
	// proper rounding is important in this formula
	// do not "optimize"
	const int toUnit = 72;
	int g = gcd(toUnit, fromUnit);
	int n = toUnit / g;
	int m = fromUnit / g;
	return (value / m) * n;
}


static int
to72dpiCeiling(int value, int fromUnit) {
	// proper rounding is important in this formula
	// do not "optimize"
	const int toUnit = 72;
	int g = gcd(toUnit, fromUnit);
	int n = toUnit / g;
	int m = fromUnit / g;
	return ((value + m - 1) / m) * n;
}


static int
from72dpi(int value, int toUnit)
{
	const int fromUnit = 72;
	return value * toUnit / fromUnit;
}


status_t
GPJob::PrintPage(list<GPBand*>& bands) {
	if (fWriteError != B_OK)
		return fWriteError;

	fPrintRect = GetPrintRectangle(bands);
	fBands = &bands;
	fCachedBand = NULL;

	{
		int left;
		int top;
		int right;
		int bottom;
		stp_get_imageable_area(fVariables, &left, &right, &bottom, &top);
		fprintf(stderr, "GPJob imageable area left %d, top %d, right %d, "
			"bottom %d\n",
			left, top, right, bottom);

	}

	{
		int left = (int)fPrintRect.left;
		int top = (int)fPrintRect.top;
		int width = fPrintRect.IntegerWidth() + 1;
		int height = fPrintRect.IntegerHeight() + 1;

		fprintf(stderr, "GPJob raw image dimensions left %d, top %d, width %d, height %d\n",
			left, top, width, height);
	}

	int xDPI = fConfiguration->fXDPI;
	int yDPI = fConfiguration->fYDPI;

	// left, top of the image on the page in 1/72 Inches
	int left = static_cast<int>(fPrintRect.left);
	left = to72dpiFloor(left, xDPI);
	int top = static_cast<int>(fPrintRect.top);
	top = to72dpiFloor(top, yDPI);

	// because of rounding in the previous step,
	// now the image left, top has to be synchronized
	fPrintRect.left = from72dpi(left, xDPI);
	fPrintRect.top = from72dpi(top, yDPI);

	// width and height of the image on the page in 1/72 Inches
	int width = fPrintRect.IntegerWidth() + 1;
	width = to72dpiCeiling(width, xDPI);
	int height = fPrintRect.IntegerHeight() + 1;
	height = to72dpiCeiling(height, yDPI);

	// synchronize image right and bottom too
	fPrintRect.right = fPrintRect.left + from72dpi(width, xDPI);
	fPrintRect.bottom = fPrintRect.top + from72dpi(height, yDPI);

	{
		int left = (int)fPrintRect.left;
		int top = (int)fPrintRect.top;
		int width = fPrintRect.IntegerWidth() + 1;
		int height = fPrintRect.IntegerHeight() + 1;

		fprintf(stderr, "GPJob image dimensions left %d, top %d, width %d, height %d\n",
			left, top, width, height);
	}

	fprintf(stderr, "GPJob image dimensions in 1/72 Inches:\n"
		"left %d, top %d, width %d, height %d\n",
		left, top, width, height);

	stp_set_width(fVariables, width);
	stp_set_height(fVariables, height);
	stp_set_left(fVariables, left);
	stp_set_top(fVariables, top);

	stp_merge_printvars(fVariables, stp_printer_get_defaults(fPrinter));

	if (!stp_verify(fVariables)) {
		// TODO report error
		fprintf(stderr, "GPJob PrintPage: invalid variables\n");
		return B_ERROR;
	}

	if (fFirstPage) {
		fFirstPage = false;
		stp_start_job(fVariables, &fImage);
	}

	stp_print(fVariables, &fImage);

	return fWriteError;
}


BRect
GPJob::GetPrintRectangle(list<GPBand*>& bands)
{
	list<GPBand*>::iterator it = bands.begin();
	if (it == bands.end())
		return BRect(0, 0, 0, 0);

	GPBand* first = *it;
	BRect rect = first->GetBoundingRectangle();
	for (it ++; it != bands.end(); it ++) {
		GPBand* band = *it;
		rect = rect | band->GetBoundingRectangle();
	}
	return rect;
}


void
GPJob::Init()
{

}


void
GPJob::Reset()
{

}


int
GPJob::Width()
{
	return static_cast<int>(fPrintRect.IntegerWidth() + 1);
}


int
GPJob::Height()
{
	return static_cast<int>(fPrintRect.IntegerHeight() + 1);
}


stp_image_status_t
GPJob::GetRow(unsigned char* data, size_t size, int row)
{
	if (fWriteError != B_OK)
		return STP_IMAGE_STATUS_ABORT;

	// row is relative to left, top of image
	// convert it to absolute value
	int line = static_cast<int>(fPrintRect.top) + row;

	FillWhite(data, size);

	GPBand* band = FindBand(line);
	if (band != NULL)
		FillRow(band, data, size, line);

	return STP_IMAGE_STATUS_OK;
}


GPBand*
GPJob::FindBand(int line)
{
	if (fCachedBand != NULL && fCachedBand->ContainsLine(line))
		return fCachedBand;

	list<GPBand*>::iterator it = fBands->begin();
	for (; it != fBands->end(); it ++) {
		GPBand* band = *it;
		if (band->ContainsLine(line)) {
			fCachedBand = band;
			return band;
		}
	}

	fCachedBand = NULL;
	return NULL;
}


void
GPJob::FillRow(GPBand* band, unsigned char* data, size_t size, int line)
{
	int imageTop = line - static_cast<int>(band->fWhere.y -
		band->fValidRect.top);
	int imageLeft = static_cast<int>(band->fValidRect.left);

	const int sourceDelta = band->fBitmap.BytesPerRow();
	const int kSourceBytesPerPixel = 4; // BGRA
	const unsigned char* source =
		static_cast<unsigned char*>(band->fBitmap.Bits())
		+ imageTop * sourceDelta
		+ imageLeft * kSourceBytesPerPixel;

	int dataLeft = static_cast<int>(band->fWhere.x - fPrintRect.left);

	const int kTargetBytesPerPixel = 3; // RGB
	unsigned char* target = &data[dataLeft * kTargetBytesPerPixel];

	const int width = band->fValidRect.IntegerWidth() + 1;
	ASSERT(0 <= imageTop && imageTop <= band->fValidRect.IntegerHeight());
	ASSERT((dataLeft + width) * kTargetBytesPerPixel <= size);

	for (int i = 0; i < width; i ++) {
		target[0] = source[2];
		target[1] = source[1];
		target[2] = source[0];
		target += kTargetBytesPerPixel;
		source += kSourceBytesPerPixel;
	}
}


void
GPJob::FillWhite(unsigned char* data, size_t size)
{
	for (size_t i = 0; i < size; i ++)
		data[i] = 0xff;
}


const char*
GPJob::GetAppname()
{
	return fApplicationName.String();
}


void
GPJob::Conclude()
{
	// nothing to do
}


void
GPJob::Write(const char* data, size_t size)
{
	try {
		fOutputStream->Write(data, size);
	} catch (TransportException e) {
		fWriteError = B_IO_ERROR;
	}
}


void
GPJob::ReportError(const char* data, size_t size)
{
	// TODO report error in printer add-on
	fprintf(stderr, "GPJob Gutenprint Error: %*s\n", (int)size, data);
}


void
GPJob::ImageInit(stp_image_t* image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	job->Init();
}


void
GPJob::ImageReset(stp_image_t* image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	job->Reset();
}


int
GPJob::ImageWidth(stp_image_t* image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	return job->Width();
}


int
GPJob::ImageHeight(stp_image_t *image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	return job->Height();
}


stp_image_status_t
GPJob::ImageGetRow(stp_image_t* image, unsigned char* data, size_t size,
	int row)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	return job->GetRow(data, size, row);
}


const char*
GPJob::ImageGetAppname(stp_image_t* image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	return job->GetAppname();
}


void
GPJob::ImageConclude(stp_image_t *image)
{
	GPJob* job = static_cast<GPJob*>(image->rep);
	job->Conclude();
}


void
GPJob::OutputFunction(void *cookie, const char *data, size_t size)
{
	GPJob* job = static_cast<GPJob*>(cookie);
	job->Write(data, size);
}


void
GPJob::ErrorFunction(void *cookie, const char *data, size_t size)
{
	GPJob* job = static_cast<GPJob*>(cookie);
	job->ReportError(data, size);
}

