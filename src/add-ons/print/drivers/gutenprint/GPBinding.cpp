/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPBinding.h"

#include <list>
#include <set>
#include <string>

#include <gutenprint/gutenprint.h>


#include "GPCapabilityExtractor.h"

using namespace std;


// printer manufacturer
static const char* kManufacturerId = "id";
static const char* kManufacturerDisplayName = "name";

// printer model
static const char* kModelDisplayName = "model";
static const char* kModelDriver = "driver";


GPBinding::GPBinding()
:
fInitialized(false),
fOutputStream(NULL)
{

}


GPBinding::~GPBinding()
{
	DeleteBands();
}


status_t
GPBinding::GetPrinterManufacturers(BMessage& manufacturers)
{
	InitGutenprint();

	list<string> ids;
	set<string> manufacturerSet;

	for (int i = 0; i < stp_printer_model_count(); i ++) {
		const stp_printer_t* printer = stp_get_printer_by_index(i);
		string manufacturer = stp_printer_get_manufacturer(printer);

		// ignore unnamed manufacturers
		if (manufacturer == "")
			continue;

		// add manufacturer only once
		if (manufacturerSet.find(manufacturer) != manufacturerSet.end())
			continue;

		manufacturerSet.insert(manufacturer);

		ids.push_back(manufacturer);
	}

	ids.sort();

	list<string>::iterator it = ids.begin();
	for (; it != ids.end(); it ++) {
		string manufacturer = *it;
		const char* id = manufacturer.c_str();
		const char* name = manufacturer.c_str();
		AddManufacturer(manufacturers, id, name);
	}
	return B_OK;
}


bool
GPBinding::ExtractManufacturer(const BMessage& manufacturers, int32 index,
	BString& id, BString& displayName)
{
	if (manufacturers.FindString(kManufacturerId, index, &id) != B_OK)
		return false;
	if (manufacturers.FindString(kManufacturerDisplayName, index, &displayName)
		!= B_OK)
		return false;
	return true;
}


void
GPBinding::AddManufacturer(BMessage& manufacturers, const char* id,
	const char* displayName)
{
	manufacturers.AddString(kManufacturerId, id);
	manufacturers.AddString(kManufacturerDisplayName, displayName);
}


status_t
GPBinding::GetPrinterModels(const char* manufacturer, BMessage& models)
{
	for (int i = 0; i < stp_printer_model_count(); i ++) {
		const stp_printer_t* printer = stp_get_printer_by_index(i);
		if (strcmp(manufacturer, stp_printer_get_manufacturer(printer)) != 0)
			continue;

		const char* displayName = stp_printer_get_long_name(printer);
		const char* driver = stp_printer_get_driver(printer);
		AddModel(models, displayName, driver);
	}
	return B_OK;
}


bool
GPBinding::ExtractModel(const BMessage& models, int32 index, BString& displayName,
	BString& driver)
{
	if (models.FindString(kModelDisplayName, index, &displayName) != B_OK)
		return false;
	if (models.FindString(kModelDriver, index, &driver) != B_OK)
		return false;
	return true;
}


void
GPBinding::AddModel(BMessage& models, const char* displayName,	const char* driver)
{
	models.AddString(kModelDisplayName, displayName);
	models.AddString(kModelDriver, driver);
}


status_t
GPBinding::GetCapabilities(const char* driver, GPCapabilities* capabilities)
{
	InitGutenprint();
	const stp_printer_t* printer = stp_get_printer_by_driver(driver);
	if (printer == NULL)
		return B_ERROR;

	GPCapabilityExtractor extractor(capabilities);
	extractor.Visit(printer);
	return B_OK;
}


status_t
GPBinding::BeginJob(GPJobConfiguration* configuration,
	OutputStream* outputStream)
{
	fOutputStream = outputStream;
	fJob.SetApplicationName("Gutenprint");
	fJob.SetConfiguration(configuration);
	fJob.SetOutputStream(outputStream);

	return fJob.Begin();
}


void
GPBinding::EndJob()
{
	fJob.End();
	fOutputStream = NULL;
}


void
GPBinding::BeginPage()
{
}


void
GPBinding::EndPage()
{
	status_t status = fJob.PrintPage(fBands);
	DeleteBands();
	if (status == B_IO_ERROR)
		throw TransportException("I/O Error");
	if (status == B_ERROR) {
		BString message;
		fJob.GetErrorMessage(message);
		throw TransportException(message.String());
	}
}


status_t
GPBinding::AddBitmapToPage(BBitmap* bitmap, BRect validRect, BPoint where)
{
	GPBand* band = new(nothrow) GPBand(bitmap, validRect, where);
	if (band == NULL) {
		return B_NO_MEMORY;
	}

	fBands.push_back(band);
	return B_OK;
}


void
GPBinding::InitGutenprint()
{
	if (fInitialized)
		return;
	fInitialized = true;
	// there is no "destroy" counter part so this creates memory leaks
	// this is no problem because the print server loads printer add-ons
	// in a new application instance that is terminated when not used anymore
	stp_init();
	stp_set_output_codeset("UTF-8");
}


void
GPBinding::DeleteBands()
{
	list<GPBand*>::iterator it = fBands.begin();
	for (; it != fBands.end(); it ++) {
		GPBand* band = *it;
		delete band;
	}
	fBands.clear();
}
