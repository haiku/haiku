/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */

#include <Printer.h>

#include <FindDirectory.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>


#include <new>


namespace BPrivate {
	namespace Print {


// TODO: remove, after pr_server.h cleanup

// mime file types
#define PSRV_PRINTER_MIMETYPE					"application/x-vnd.Be.printer"


// printer attributes
#define PSRV_PRINTER_ATTR_STATE					"state"
#define PSRV_PRINTER_ATTR_COMMENTS				"Comments"
#define PSRV_PRINTER_ATTR_TRANSPORT				"transport"
#define PSRV_PRINTER_ATTR_DRIVER_NAME			"Driver Name"
#define PSRV_PRINTER_ATTR_PRINTER_NAME			"Printer Name"
#define PSRV_PRINTER_ATTR_DEFAULT_PRINTER		"Default Printer"
#define PSRV_PRINTER_ATTR_TRANSPORT_ADDRESS		"transport_address"


// message fields
#define PSRV_FIELD_CURRENT_PRINTER				"current_printer"


BPrinter::BPrinter()
	: fListener(NULL)
{
	memset(&fPrinterEntryRef, 0, sizeof(entry_ref));
}


BPrinter::BPrinter(const BEntry& entry)
	: fListener(NULL)
{
	SetTo(entry);
}


BPrinter::BPrinter(const BPrinter& printer)
{
	*this = printer;
}


BPrinter::BPrinter(const node_ref& nodeRef)
	: fListener(NULL)
{
	SetTo(nodeRef);
}


BPrinter::BPrinter(const entry_ref& entryRef)
	: fListener(NULL)
	, fPrinterEntryRef(entryRef)
{
}


BPrinter::BPrinter(const BDirectory& directory)
	: fListener(NULL)
{
	SetTo(directory);
}


BPrinter::~BPrinter()
{
	StopWatching();
}


status_t
BPrinter::SetTo(const BEntry& entry)
{
	StopWatching();
	entry.GetRef(&fPrinterEntryRef);

	return InitCheck();
}


status_t
BPrinter::SetTo(const node_ref& nodeRef)
{
	SetTo(BDirectory(&nodeRef));
	return InitCheck();
}


status_t
BPrinter::SetTo(const entry_ref& entryRef)
{
	StopWatching();
	fPrinterEntryRef = entryRef;

	return InitCheck();
}


status_t
BPrinter::SetTo(const BDirectory& directory)
{
	StopWatching();

	BEntry entry;
	directory.GetEntry(&entry);
	entry.GetRef(&fPrinterEntryRef);

	return InitCheck();
}


void
BPrinter::Unset()
{
	StopWatching();
	memset(&fPrinterEntryRef, 0, sizeof(entry_ref));
}


bool
BPrinter::IsValid() const
{
	BDirectory spoolDir(&fPrinterEntryRef);
	if (spoolDir.InitCheck() != B_OK)
		return false;

	BNode node(spoolDir);
	char type[B_MIME_TYPE_LENGTH];
	BNodeInfo(&node).GetType(type);

	if (strcmp(type, PSRV_PRINTER_MIMETYPE) != 0)
		return false;

	return true;
}


status_t
BPrinter::InitCheck() const
{
	BDirectory spoolDir(&fPrinterEntryRef);
	return spoolDir.InitCheck();
}


bool
BPrinter::IsFree() const
{
	return (State() == "free");
}


bool
BPrinter::IsDefault() const
{
	bool isDefault = false;

	BDirectory spoolDir(&fPrinterEntryRef);
	if (spoolDir.InitCheck() == B_OK)
		spoolDir.ReadAttr(PSRV_PRINTER_ATTR_DEFAULT_PRINTER, B_BOOL_TYPE, 0,
			&isDefault, sizeof(bool));

	return isDefault;
}


bool
BPrinter::IsShareable() const
{
	if (Name() == "Preview")
		return true;

	return false;
}


BString
BPrinter::Name() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_PRINTER_NAME);
}


BString
BPrinter::State() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_STATE);
}


BString
BPrinter::Driver() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_DRIVER_NAME);
}


BString
BPrinter::Comments() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_COMMENTS);
}


BString
BPrinter::Transport() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_TRANSPORT);
}


BString
BPrinter::TransportAddress() const
{
	return _ReadAttribute(PSRV_PRINTER_ATTR_TRANSPORT_ADDRESS);
}


status_t
BPrinter::DefaultSettings(BMessage& settings)
{
	status_t status = B_ERROR;
	image_id id = _LoadDriver();
	if (id < 0)
		return status;

	typedef BMessage* (*default_settings_func_t)(BNode*);
	default_settings_func_t default_settings;
	if (get_image_symbol(id, "default_settings", B_SYMBOL_TYPE_TEXT
		, (void**)&default_settings) == B_OK) {
		BNode printerNode(&fPrinterEntryRef);
		BMessage *newSettings = default_settings(&printerNode);
		if (newSettings) {
			status = B_OK;
			settings = *newSettings;
			_AddPrinterName(settings);
		}
		delete newSettings;
	}
	unload_add_on(id);
	return status;
}


status_t
BPrinter::StartWatching(const BMessenger& listener)
{
	StopWatching();

	if (!listener.IsValid())
		return B_BAD_VALUE;

	fListener = new(std::nothrow) BMessenger(listener);
	if (!fListener)
		return B_NO_MEMORY;

	node_ref nodeRef;
	nodeRef.device = fPrinterEntryRef.device;
	nodeRef.node = fPrinterEntryRef.directory;

	return watch_node(&nodeRef, B_WATCH_DIRECTORY, *fListener);
}


void
BPrinter::StopWatching()
{
	if (fListener) {
		stop_watching(*fListener);
		delete fListener;
		fListener = NULL;
	}
}


BPrinter&
BPrinter::operator=(const BPrinter& printer)
{
	if (this != &printer) {
		Unset();
		fPrinterEntryRef = printer.fPrinterEntryRef;
		if (printer.fListener)
			StartWatching(*printer.fListener);
	}
	return *this;
}


bool
BPrinter::operator==(const BPrinter& printer) const
{
	return (fPrinterEntryRef == printer.fPrinterEntryRef);
}


bool
BPrinter::operator!=(const BPrinter& printer) const
{
	return (fPrinterEntryRef != printer.fPrinterEntryRef);
}


status_t
BPrinter::_Configure() const
{
	status_t status = B_ERROR;
	image_id id = _LoadDriver();
	if (id < 0)
		return status;

	BString printerName(_ReadAttribute(PSRV_PRINTER_ATTR_PRINTER_NAME));
	if (printerName.Length() > 0) {
		typedef char* (*add_printer_func_t)(const char*);
		add_printer_func_t add_printer;
		if (get_image_symbol(id, "add_printer", B_SYMBOL_TYPE_TEXT
			, (void**)&add_printer) == B_OK) {
				if (add_printer(printerName.String()) != NULL)
					status = B_OK;
		}
	} else {
		status = B_ERROR;
	}
	unload_add_on(id);
	return status;
}


status_t
BPrinter::_ConfigureJob(BMessage& settings)
{
	status_t status = B_ERROR;
	image_id id = _LoadDriver();
	if (id < 0)
		return status;

	typedef BMessage* (*config_job_func_t)(BNode*, const BMessage*);
	config_job_func_t configure_job;
	if (get_image_symbol(id, "config_job", B_SYMBOL_TYPE_TEXT
		, (void**)&configure_job) == B_OK) {
		BNode printerNode(&fPrinterEntryRef);
		BMessage *newSettings = configure_job(&printerNode, &settings);
		if (newSettings && (newSettings->what == 'okok')) {
			status = B_OK;
			settings = *newSettings;
			_AddPrinterName(settings);
		}
		delete newSettings;
	}
	unload_add_on(id);
	return status;
}


status_t
BPrinter::_ConfigurePage(BMessage& settings)
{
	status_t status = B_ERROR;
	image_id id = _LoadDriver();
	if (id < 0)
		return status;

	typedef BMessage* (*config_page_func_t)(BNode*, const BMessage*);
	config_page_func_t configure_page;
	if (get_image_symbol(id, "config_page", B_SYMBOL_TYPE_TEXT
		, (void**)&configure_page) == B_OK) {
		BNode printerNode(&fPrinterEntryRef);
		BMessage *newSettings = configure_page(&printerNode, &settings);
		if (newSettings && (newSettings->what == 'okok')) {
			status = B_OK;
			settings = *newSettings;
			_AddPrinterName(settings);
		}
		delete newSettings;
	}
	unload_add_on(id);
	return status;
}


BPath
BPrinter::_DriverPath() const
{
	BString driverName(_ReadAttribute(PSRV_PRINTER_ATTR_DRIVER_NAME));
	if (driverName.Length() <= 0)
		return BPath();

	directory_which directories[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_NONPACKAGED_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};

	BPath path;
	driverName.Prepend("Print/");
	for (int32 i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i) {
		if (find_directory(directories[i], &path) == B_OK) {
			path.Append(driverName.String());

			BEntry driver(path.Path());
			if (driver.InitCheck() == B_OK && driver.Exists() && driver.IsFile())
				return path;
		}
	}
	return BPath();
}


image_id
BPrinter::_LoadDriver() const
{
	BPath driverPath(_DriverPath());
	if (driverPath.InitCheck() != B_OK)
		return -1;

	return load_add_on(driverPath.Path());
}


void
BPrinter::_AddPrinterName(BMessage& settings)
{
	settings.RemoveName(PSRV_FIELD_CURRENT_PRINTER);
	settings.AddString(PSRV_FIELD_CURRENT_PRINTER, Name());
}


BString
BPrinter::_ReadAttribute(const char* attribute) const
{
	BString value;

	BDirectory spoolDir(&fPrinterEntryRef);
	if (spoolDir.InitCheck() == B_OK)
		spoolDir.ReadAttrString(attribute, &value);

	return value;
}


	}	// namespace Print
}	// namespace BPrivate
