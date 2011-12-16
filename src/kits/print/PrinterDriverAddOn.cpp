/*
 * Copyright 2001-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#include "PrinterDriverAddOn.h"

#include <File.h>

#include "BeUtils.h"
#include "pr_server.h"


typedef BMessage* (*config_func_t)(BNode*, const BMessage*);
typedef BMessage* (*take_job_func_t)(BFile*, BNode*, const BMessage*);
typedef char* (*add_printer_func_t)(const char* printer_name);
typedef BMessage* (*default_settings_t)(BNode*);

static const char* kPrinterDriverFolderName = "Print";


PrinterDriverAddOn::PrinterDriverAddOn(const char* driver)
	:
	fAddOnID(-1)
{
	BPath path;
	status_t result;
	result = FindPathToDriver(driver, &path);
	if (result != B_OK)
		return;

	fAddOnID = ::load_add_on(path.Path());
}


PrinterDriverAddOn::~PrinterDriverAddOn()
{
	if (IsLoaded()) {
		unload_add_on(fAddOnID);
		fAddOnID = -1;
	}
}


status_t
PrinterDriverAddOn::AddPrinter(const char* spoolFolderName)
{
	if (!IsLoaded())
		return B_ERROR;

	add_printer_func_t func;
	status_t result = get_image_symbol(fAddOnID, "add_printer",
		B_SYMBOL_TYPE_TEXT, (void**)&func);
	if (result != B_OK)
		return result;

	if ((*func)(spoolFolderName) == NULL)
		return B_ERROR;
	return B_OK;
}


status_t
PrinterDriverAddOn::ConfigPage(BDirectory* spoolFolder, BMessage* settings)
{
	if (!IsLoaded())
		return B_ERROR;

	config_func_t func;
	status_t result = get_image_symbol(fAddOnID, "config_page",
		B_SYMBOL_TYPE_TEXT, (void**)&func);
	if (result != B_OK)
		return result;

	BMessage* newSettings = (*func)(spoolFolder, settings);
	result = CopyValidSettings(settings, newSettings);
	delete newSettings;

	return result;
}


status_t
PrinterDriverAddOn::ConfigJob(BDirectory* spoolFolder, BMessage* settings)
{
	if (!IsLoaded())
		return B_ERROR;

	config_func_t func;
	status_t result = get_image_symbol(fAddOnID, "config_job",
		B_SYMBOL_TYPE_TEXT, (void**)&func);
	if (result != B_OK)
		return result;

	BMessage* newSettings = (*func)(spoolFolder, settings);
	result = CopyValidSettings(settings, newSettings);
	delete newSettings;

	return result;
}


status_t
PrinterDriverAddOn::DefaultSettings(BDirectory* spoolFolder, BMessage* settings)
{
	if (!IsLoaded())
		return B_ERROR;

	default_settings_t func;
	status_t result = get_image_symbol(fAddOnID, "default_settings",
		B_SYMBOL_TYPE_TEXT, (void**)&func);
	if (result != B_OK)
		return result;

	BMessage* newSettings = (*func)(spoolFolder);
	if (newSettings != NULL) {
		*settings = *newSettings;
		settings->what = 'okok';
	} else
		result = B_ERROR;
	delete newSettings;

	return result;
}


status_t
PrinterDriverAddOn::TakeJob(const char* spoolFile, BDirectory* spoolFolder)
{
	if (!IsLoaded())
		return B_ERROR;

	BFile file(spoolFile, B_READ_WRITE);
	take_job_func_t func;
	status_t result = get_image_symbol(fAddOnID, "take_job", B_SYMBOL_TYPE_TEXT,
		(void**)&func);
	if (result != B_OK)
		return result;

	// This seems to be required for legacy?
	// HP PCL3 add-on crashes without it!
	BMessage parameters(B_REFS_RECEIVED);
	parameters.AddInt32("file", (int32)&file);
	parameters.AddInt32("printer", (int32)spoolFolder);

	BMessage* message = (*func)(&file, spoolFolder, &parameters);
	if (message == NULL || message->what != 'okok')
		result = B_ERROR;
	delete message;

	return result;
}


status_t
PrinterDriverAddOn::FindPathToDriver(const char* driver, BPath* path)
{
	status_t result;
	result = ::TestForAddonExistence(driver, B_USER_ADDONS_DIRECTORY,
		kPrinterDriverFolderName, *path);
	if (result == B_OK)
		return B_OK;

	result = ::TestForAddonExistence(driver, B_COMMON_ADDONS_DIRECTORY,
		kPrinterDriverFolderName, *path);
	if (result == B_OK)
		return B_OK;

	result = ::TestForAddonExistence(driver, B_SYSTEM_ADDONS_DIRECTORY,
		kPrinterDriverFolderName, *path);
	return result;
}


bool
PrinterDriverAddOn::IsLoaded() const
{
	return fAddOnID > 0;
}


status_t
PrinterDriverAddOn::CopyValidSettings(BMessage* settings, BMessage* newSettings)
{
	if (newSettings != NULL && newSettings->what != 'baad') {
		*settings = *newSettings;
		settings->what = 'okok';
		return B_OK;
	}
	return B_ERROR;
}
