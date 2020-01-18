/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPJob.h"

#include <Debug.h>


// 72 DPI
static const int32 kGutenprintUnit = 72;

class CoordinateSystem
{
public:
	CoordinateSystem()
	:
	fXDPI(0),
	fYDPI(0)
	{
	}


	void SetDPI(int32 x, int32 y) {
		fXDPI = x;
		fYDPI = y;
	}


	void ToGutenprint(int32 fromX, int32 fromY, double& toX, double& toY) {
		toX = fromX * kGutenprintUnit / fXDPI;
		toY = fromY * kGutenprintUnit / fYDPI;
	}


	void ToGutenprintCeiling(int32 fromX, int32 fromY, double& toX,
		double& toY) {
		toX = (fromX * kGutenprintUnit + fXDPI - 1) / fXDPI;
		toY = (fromY * kGutenprintUnit + fYDPI - 1) / fYDPI;
	}


	void FromGutenprint(double fromX, double fromY, int32& toX, int32& toY) {
		toX = (int32)(fromX * fXDPI / kGutenprintUnit);
		toY = (int32)(fromY * fYDPI / kGutenprintUnit);
	}

	void FromGutenprintCeiling(double fromX, double fromY, int32& toX,
		int32& toY) {
		toX = (int32)((fromX * fXDPI + kGutenprintUnit - 1) / kGutenprintUnit);
		toY = (int32)((fromY * fYDPI + kGutenprintUnit - 1) / kGutenprintUnit);
	}

	void SizeFromGutenprint(double fromWidth, double fromHeight,
		int32& toWidth, int32& toHeight) {
		toWidth = (int32)(fromWidth * fXDPI / kGutenprintUnit);
		toHeight = (int32)(fromHeight * fYDPI / kGutenprintUnit);
	}

	void RoundUpToWholeInches(int32& width, int32& height) {
		width = ((width + kGutenprintUnit - 1) / kGutenprintUnit)
			* kGutenprintUnit;
		height = ((height + kGutenprintUnit - 1) / kGutenprintUnit)
			* kGutenprintUnit;
	}

private:
	double fXDPI;
	double fYDPI;
};


GPJob::GPJob()
	:
	fApplicationName(),
	fOutputStream(NULL),
	fConfiguration(NULL),
	fHasPages(false),
	fVariables(NULL),
	fPrinter(NULL),
	fBands(NULL),
	fCachedBand(NULL),
	fStatus(B_OK)
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
	fStatus = B_OK;

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

	if (fHasPages)
		stp_end_job(fVariables, &fImage);

	stp_vars_destroy(fVariables);
	fVariables = NULL;
}

status_t
GPJob::PrintPage(list<GPBand*>& bands) {
	if (fStatus != B_OK)
		return fStatus;

	fBands = &bands;
	fCachedBand = NULL;

	Rectangle<stp_dimension_t> imageableArea;
	stp_get_imageable_area(fVariables, &imageableArea.left,
		&imageableArea.right, &imageableArea.bottom, &imageableArea.top);
	fprintf(stderr, "GPJob imageable area left %f, top %f, right %f, "
		"bottom %f\n",
		imageableArea.left, imageableArea.top, imageableArea.right,
		imageableArea.bottom);
	fprintf(stderr, "GPJob width %f %s, height %f %s\n",
		imageableArea.Width(),
		std::fmod(imageableArea.Width(), 72.) == 0.0 ? "whole inches" : "not whole inches",
		imageableArea.Height(),
		std::fmod(imageableArea.Height(), 72.) == 0.0 ? "whole inches" : "not whole inches"
		);

	CoordinateSystem coordinateSystem;
	coordinateSystem.SetDPI(fConfiguration->fXDPI, fConfiguration->fYDPI);
	{
		// GPBand offset is relative to imageable area left, top
		// but it has to be absolute to left, top of page
		int32 offsetX;
		int32 offsetY;
		coordinateSystem.FromGutenprintCeiling(imageableArea.left,
			imageableArea.top, offsetX, offsetY);

		BPoint offset(offsetX, offsetY);
		list<GPBand*>::iterator it = fBands->begin();
		for (; it != fBands->end(); it++) {
			(*it)->fWhere += offset;
		}
	}

	fPrintRect = GetPrintRectangle(bands);

	{
		int left = (int)fPrintRect.left;
		int top = (int)fPrintRect.top;
		int width = fPrintRect.Width() + 1;
		int height = fPrintRect.Height() + 1;

		fprintf(stderr, "GPJob bitmap bands frame left %d, top %d, width %d, "
			"height %d\n",
			left, top, width, height);
	}

	// calculate the position and size of the image to be printed on the page
	// unit: 1/72 Inches
	// constraints: the image must be inside the imageable area
	double left;
	double top;
	coordinateSystem.ToGutenprint(fPrintRect.left, fPrintRect.top, left, top);
	if (left < imageableArea.left)
		left = imageableArea.left;
	if (top < imageableArea.top)
		top = imageableArea.top;

	double right;
	double bottom;
	coordinateSystem.ToGutenprintCeiling(fPrintRect.right, fPrintRect.bottom,
		right, bottom);
	if (right > imageableArea.right)
		right = imageableArea.right;
	if (bottom > imageableArea.bottom)
		bottom = imageableArea.bottom;

	double width = right - left;
	double height = bottom - top;

	// because of rounding and clipping in the previous step,
	// now the image frame has to be synchronized
	coordinateSystem.FromGutenprint(left, top, fPrintRect.left, fPrintRect.top);
	int32 printRectWidth;
	int32 printRectHeight;
	coordinateSystem.SizeFromGutenprint(width, height, printRectWidth,
		printRectHeight);
	fPrintRect.right = fPrintRect.left + printRectWidth - 1;
	fPrintRect.bottom = fPrintRect.top + printRectHeight - 1;
	{
		int left = fPrintRect.left;
		int top = fPrintRect.top;
		int width = fPrintRect.Width() + 1;
		int height = fPrintRect.Height() + 1;

		fprintf(stderr, "GPJob image dimensions left %d, top %d, width %d, "
			"height %d\n",
			left, top, width, height);
	}

	fprintf(stderr, "GPJob image dimensions in 1/72 Inches: "
		"left %d, top %d, right %d, bottom %d\n",
		(int)left, (int)top, (int)right, (int)bottom);

	stp_set_width(fVariables, right - left);
	stp_set_height(fVariables, bottom - top);
	stp_set_left(fVariables, left);
	stp_set_top(fVariables, top);

	stp_merge_printvars(fVariables, stp_printer_get_defaults(fPrinter));

	if (!stp_verify(fVariables)) {
		fprintf(stderr, "GPJob PrintPage: invalid variables\n");
		return B_ERROR;
	}

	if (!fHasPages) {
		fHasPages = true;
		stp_start_job(fVariables, &fImage);
	}

	stp_print(fVariables, &fImage);

	return fStatus;
}


void
GPJob::GetErrorMessage(BString& message)
{
	message = fErrorMessage;
}


RectInt32
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
	return fPrintRect.Width() + 1;
}


int
GPJob::Height()
{
	return fPrintRect.Height() + 1;
}


stp_image_status_t
GPJob::GetRow(unsigned char* data, size_t size, int row)
{
	if (fStatus != B_OK)
		return STP_IMAGE_STATUS_ABORT;

	// row is relative to left, top of image
	// convert it to absolute y coordinate value
	int line = fPrintRect.top + row;

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

	const int sourceBytesPerRow = band->fBitmap.BytesPerRow();
	const int kSourceBytesPerPixel = 4; // BGRA
	const unsigned char* source =
		static_cast<unsigned char*>(band->fBitmap.Bits())
		+ imageTop * sourceBytesPerRow
		+ imageLeft * kSourceBytesPerPixel;

	int dataLeft = static_cast<int>(band->fWhere.x - fPrintRect.left);
	int sourcePixelsToSkip = 0;
	if (dataLeft < 0) {
		sourcePixelsToSkip = -dataLeft;
		dataLeft = 0;
	}
	int width = band->fValidRect.IntegerWidth() + 1 - sourcePixelsToSkip;
	source += sourcePixelsToSkip * kSourceBytesPerPixel;
	if (width <= 0)
		return;

	const int kTargetBytesPerPixel = 3; // RGB
	unsigned char* target = &data[dataLeft * kTargetBytesPerPixel];
	int maxWidth = size / kTargetBytesPerPixel - dataLeft;
	if (width > maxWidth)
		width = maxWidth;

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
	} catch (TransportException& e) {
		fStatus = B_IO_ERROR;
	}
}


void
GPJob::ReportError(const char* data, size_t size)
{
	if (fStatus == B_OK)
		fStatus = B_ERROR;
	fErrorMessage.Append(data, size);
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

