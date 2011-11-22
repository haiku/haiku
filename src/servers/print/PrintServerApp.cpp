/*
 * Copyright 2001-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#include "PrintServerApp.h"

#include "BeUtils.h"
#include "Printer.h"
#include "pr_server.h"
#include "Transport.h"

// BeOS API
#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <image.h>
#include <Locale.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <PrintJob.h>
#include <String.h>

// ANSI C
#include <stdio.h>
	// for printf
#include <unistd.h>
	// for unlink


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PrintServerApp"


typedef struct _printer_data {
	char defaultPrinterName[256];
} printer_data_t;



static const char* kSettingsName = "print_server_settings";


BLocker *gLock = NULL;


/**
 * Main entry point of print_server.
 *
 * @returns B_OK if application was started, or an errorcode if
 *          application failed to start.
 */
int
main()
{
	status_t rc = B_OK;
	gLock = new BLocker();
	PrintServerApp print_server(&rc);
	if (rc == B_OK) {
		print_server.Run();
	}
	delete gLock;
	return rc;
}


/**
 * Constructor for print_server's application class. Retrieves the
 * name of the default printer from storage, caches the icons for
 * a selected printer.
 *
 * @param err Pointer to status_t for storing result of application
 *          initialisation.
 *
 * @see BApplication
 */
PrintServerApp::PrintServerApp(status_t* err)
	: Inherited(PSRV_SIGNATURE_TYPE, err),
	fDefaultPrinter(NULL),
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fIconSize(0),
	fSelectedIcon(NULL),
#else
	fSelectedIconMini(NULL),
	fSelectedIconLarge(NULL),
#endif
	fReferences(0),
	fHasReferences(0),
	fUseConfigWindow(true),
	fFolder(NULL)
{
	fSettings = Settings::GetSettings();
	LoadSettings();

	if (*err != B_OK)
		return;

	fHasReferences = create_sem(1, "has_references");

	// Build list of transport addons
	Transport::Scan(B_USER_ADDONS_DIRECTORY);
	Transport::Scan(B_COMMON_ADDONS_DIRECTORY);
	Transport::Scan(B_SYSTEM_ADDONS_DIRECTORY);

	SetupPrinterList();
	RetrieveDefaultPrinter();

	// Cache icons for selected printer
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	BMimeType type(PSRV_PRINTER_FILETYPE);
	type.GetIcon(&fSelectedIcon, &fIconSize);
#else
	fSelectedIconMini = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1), B_CMAP8);
	fSelectedIconLarge = new BBitmap(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8);
	BMimeType type(PRNT_SIGNATURE_TYPE);
	type.GetIcon(fSelectedIconMini, B_MINI_ICON);
	type.GetIcon(fSelectedIconLarge, B_LARGE_ICON);
#endif

	PostMessage(PSRV_PRINT_SPOOLED_JOB);
		// Start handling of spooled files
}


PrintServerApp::~PrintServerApp()
{
	SaveSettings();
	delete fSettings;
}


bool
PrintServerApp::QuitRequested()
{
	// don't quit when user types Command+Q!
	BMessage* m = CurrentMessage();
	bool shortcut;
	if (m != NULL && m->FindBool("shortcut", &shortcut) == B_OK && shortcut)
		return false;

	if (!Inherited::QuitRequested())
		return false;

	// Stop watching the folder
	delete fFolder; fFolder = NULL;

	// Release all printers
	Printer* printer;
	while ((printer = Printer::At(0)) != NULL) {
		printer->AbortPrintThread();
		UnregisterPrinter(printer);
	}

	// Wait for printers
	if (fHasReferences > 0) {
		acquire_sem(fHasReferences);
		delete_sem(fHasReferences);
		fHasReferences = 0;
	}

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	delete [] fSelectedIcon;
	fSelectedIcon = NULL;
#else
	delete fSelectedIconMini;
	fSelectedIconMini = NULL;
	delete fSelectedIconLarge;
	fSelectedIconLarge = NULL;
#endif

	return true;
}


void
PrintServerApp::Acquire()
{
	if (atomic_add(&fReferences, 1) == 0)
		acquire_sem(fHasReferences);
}


void
PrintServerApp::Release()
{
	if (atomic_add(&fReferences, -1) == 1)
		release_sem(fHasReferences);
}


void
PrintServerApp::RegisterPrinter(BDirectory* printer)
{
	BString transport, address, connection, state;

	if (printer->ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT, &transport) == B_OK
		&& printer->ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, &address)
			== B_OK
		&& printer->ReadAttrString(PSRV_PRINTER_ATTR_CNX, &connection) == B_OK
		&& printer->ReadAttrString(PSRV_PRINTER_ATTR_STATE, &state) == B_OK
		&& state == "free") {

 		BAutolock lock(gLock);
		if (lock.IsLocked()) {
			// check if printer is already registered
			node_ref node;
			if (printer->GetNodeRef(&node) != B_OK)
				return;

			if (Printer::Find(&node) != NULL)
				return;

			// register new printer
			Resource* r = fResourceManager.Allocate(transport.String(),
				address.String(), connection.String());
		 	AddHandler(new Printer(printer, r));
		 	Acquire();
		}
	}
}


void
PrintServerApp::UnregisterPrinter(Printer* printer)
{
	RemoveHandler(printer);
	Printer::Remove(printer);
	printer->Release();
}


void
PrintServerApp::NotifyPrinterDeletion(Printer* printer)
{
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		fResourceManager.Free(printer->GetResource());
		Release();
	}
}


void
PrintServerApp::EntryCreated(node_ref* node, entry_ref* entry)
{
	BEntry printer(entry);
	if (printer.InitCheck() == B_OK && printer.IsDirectory()) {
		BDirectory dir(&printer);
		if (dir.InitCheck() == B_OK) RegisterPrinter(&dir);
	}
}


void
PrintServerApp::EntryRemoved(node_ref* node)
{
	Printer* printer = Printer::Find(node);
	if (printer) {
		if (printer == fDefaultPrinter) fDefaultPrinter = NULL;
		UnregisterPrinter(printer);
	}
}


void
PrintServerApp::AttributeChanged(node_ref* node)
{
	BDirectory printer(node);
	if (printer.InitCheck() == B_OK) {
		RegisterPrinter(&printer);
	}
}


// ---------------------------------------------------------------
// SetupPrinterList
//
// This method builds the internal list of printers from disk. It
// also installs a node monitor to be sure that the list keeps
// updated with the definitions on disk.
//
// Parameters:
//    none.
//
// Returns:
//    B_OK if successful, or an errorcode if failed.
// ---------------------------------------------------------------
status_t
PrintServerApp::SetupPrinterList()
{
	status_t rc;

	// Find directory containing printer definition nodes
	BPath path;
	rc = ::find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	if (rc != B_OK)
		return rc;

	// Directory has to exist in order to watch it
	mode_t mode = 0777;
	create_directory(path.Path(), mode);

	BDirectory dir(path.Path());
	rc = dir.InitCheck();
	if (rc != B_OK)
		return rc;

	// Register printer definition nodes
	BEntry entry;
	while(dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;

		BDirectory node(&entry);
		BNodeInfo info(&node);
		char buffer[256];
		if (info.GetType(buffer) == B_OK
			&& strcmp(buffer, PSRV_PRINTER_FILETYPE) == 0) {
			RegisterPrinter(&node);
		}
	}

	// Now we are ready to start node watching
	fFolder = new FolderWatcher(this, dir, true);
	fFolder->SetListener(this);

	return B_OK;
}

// ---------------------------------------------------------------
// void MessageReceived(BMessage* msg)
//
// Message handling method for print_server application class.
//
// Parameters:
//      msg - Actual message sent to application class.
//
// Returns:
//      void.
// ---------------------------------------------------------------
void
PrintServerApp::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case PSRV_GET_ACTIVE_PRINTER:
		case PSRV_MAKE_PRINTER_ACTIVE_QUIETLY:
		case PSRV_MAKE_PRINTER_ACTIVE:
		case PSRV_MAKE_PRINTER:
		case PSRV_SHOW_PAGE_SETUP:
		case PSRV_SHOW_PRINT_SETUP:
		case PSRV_PRINT_SPOOLED_JOB:
		case PSRV_GET_DEFAULT_SETTINGS:
			Handle_BeOSR5_Message(msg);
			break;

		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		case B_CREATE_PROPERTY:
		case B_DELETE_PROPERTY:
		case B_COUNT_PROPERTIES:
		case B_EXECUTE_PROPERTY:
			HandleScriptingCommand(msg);
			break;

		default:
			Inherited::MessageReceived(msg);
	}
}


// ---------------------------------------------------------------
// CreatePrinter(const char* printerName, const char* driverName,
//               const char* connection, const char* transportName,
//               const char* transportPath)
//
// Creates printer definition/spool directory. It sets the
// attributes of the directory to the values passed and calls
// the driver's add_printer method to handle any configuration
// needed.
//
// Parameters:
//    printerName - Name of printer to create.
//    driverName - Name of driver to use for this printer.
//    connection - "Local" or "Network".
//    transportName - Name of transport driver to use.
//    transportPath  - Configuration data for transport driver.
//
// Returns:
// ---------------------------------------------------------------
status_t
PrintServerApp::CreatePrinter(const char* printerName, const char* driverName,
	const char* connection, const char* transportName,
	const char* transportPath)
{
	status_t rc;

	// Find directory containing printer definitions
	BPath path;
	rc = ::find_directory(B_USER_PRINTERS_DIRECTORY,&path,true,NULL);
	if (rc != B_OK)
		return rc;

	// Create our printer definition/spool directory
	BDirectory printersDir(path.Path());
	BDirectory printer;

	rc = printersDir.CreateDirectory(printerName, &printer);
	if (rc == B_FILE_EXISTS) {
		printer.SetTo(&printersDir, printerName);

		BString info;
		char type[B_MIME_TYPE_LENGTH];
		BNodeInfo(&printer).GetType(type);
		if (strcmp(PSRV_PRINTER_FILETYPE, type) == 0) {
			BPath path;
			if (Printer::FindPathToDriver(printerName, &path) == B_OK) {
				if (fDefaultPrinter) {
					// the printer exists, but is not the default printer
					if (strcmp(fDefaultPrinter->Name(), printerName) != 0)
						rc = B_OK;
					return rc;
				}
				// the printer exists, but no default at all
				return B_OK;
			} else {
				info.SetTo(B_TRANSLATE(
					"A printer with that name already exists, "
					"but its driver could not be found! Replace?"));
			}
		} else {
			info.SetTo(B_TRANSLATE(
				"A printer with that name already exists, "
				"but it's not usable at all! Replace?"));
		}

		if (info.Length() != 0) {
			BAlert *alert = new BAlert("Info", info.String(),
				B_TRANSLATE("Cancel"), B_TRANSLATE("OK"));
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 0)
				return rc;
		}
	} else if (rc != B_OK) {
		return rc;
	}

	// Set its type to a printer
	BNodeInfo info(&printer);
	info.SetType(PSRV_PRINTER_FILETYPE);

	// Store the settings in its attributes
	printer.WriteAttr(PSRV_PRINTER_ATTR_PRT_NAME, B_STRING_TYPE, 0, printerName,
		::strlen(printerName) + 1);
	printer.WriteAttr(PSRV_PRINTER_ATTR_DRV_NAME, B_STRING_TYPE, 0, driverName,
		::strlen(driverName) + 1);
	printer.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT, B_STRING_TYPE, 0,
		transportName, ::strlen(transportName) + 1);
	printer.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0,
		transportPath, ::strlen(transportPath) + 1);
	printer.WriteAttr(PSRV_PRINTER_ATTR_CNX, B_STRING_TYPE, 0, connection,
		::strlen(connection) + 1);

	rc = Printer::ConfigurePrinter(driverName, printerName);
	if (rc == B_OK) {
		// Notify printer driver that a new printer definition node
		// has been created.
		printer.WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE, 0, "free",
			::strlen("free")+1);
	}

	if (rc != B_OK) {
		BEntry entry;
		if (printer.GetEntry(&entry) == B_OK)
			entry.Remove();
	}

	return rc;
}


// ---------------------------------------------------------------
// SelectPrinter(const char* printerName)
//
// Makes a new printer the active printer. This is done simply
// by changing our class attribute fDefaultPrinter, and changing
// the icon of the BNode for the printer. Ofcourse, we need to
// change the icon of the "old" default printer first back to a
// "non-active" printer icon first.
//
// Parameters:
//		printerName - Name of the new active printer.
//
// Returns:
//      B_OK on success, or error code otherwise.
// ---------------------------------------------------------------
status_t
PrintServerApp::SelectPrinter(const char* printerName)
{
	status_t rc;
	BNode node;

	// Find the node of the "old" default printer
	if (fDefaultPrinter != NULL
		&& FindPrinterNode(fDefaultPrinter->Name(), node) == B_OK) {
		// and remove the custom icon
		BNodeInfo info(&node);
		info.SetIcon(NULL, B_MINI_ICON);
		info.SetIcon(NULL, B_LARGE_ICON);
	}

	// Find the node for the new default printer
	rc=FindPrinterNode(printerName, node);
	if (rc == B_OK) {
		// and add the custom icon
		BNodeInfo info(&node);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		info.SetIcon(fSelectedIcon, fIconSize);
#else
		info.SetIcon(fSelectedIconMini, B_MINI_ICON);
		info.SetIcon(fSelectedIconLarge, B_LARGE_ICON);
#endif
	}

	fDefaultPrinter = Printer::Find(printerName);
	StoreDefaultPrinter();
		// update our pref file
	be_roster->Broadcast(new BMessage(B_PRINTER_CHANGED));

	return rc;
}


// ---------------------------------------------------------------
// HandleSpooledJobs()
//
// Handles calling the printer drivers for printing a spooled job.
//
// ---------------------------------------------------------------
void
PrintServerApp::HandleSpooledJobs()
{
	const int n = Printer::CountPrinters();
	for (int i = 0; i < n; i ++) {
		Printer* printer = Printer::At(i);
		printer->HandleSpooledJob();
	}
}


// ---------------------------------------------------------------
// RetrieveDefaultPrinter()
//
// Loads the currently selected printer from a private settings
// file.
//
// Parameters:
//     none.
//
// Returns:
//     Error code on failore, or B_OK if all went fine.
// ---------------------------------------------------------------
status_t
PrintServerApp::RetrieveDefaultPrinter()
{
	fDefaultPrinter = Printer::Find(fSettings->DefaultPrinter());
	return B_OK;
}


// ---------------------------------------------------------------
// StoreDefaultPrinter()
//
// Stores the currently selected printer in a private settings
// file.
//
// Parameters:
//     none.
//
// Returns:
//     Error code on failore, or B_OK if all went fine.
// ---------------------------------------------------------------
status_t
PrintServerApp::StoreDefaultPrinter()
{
	if (fDefaultPrinter)
		fSettings->SetDefaultPrinter(fDefaultPrinter->Name());
	else
		fSettings->SetDefaultPrinter("");
	return B_OK;
}


// ---------------------------------------------------------------
// FindPrinterNode(const char* name, BNode& node)
//
// Find the BNode representing the specified printer. It searches
// *only* in the users printer definitions.
//
// Parameters:
//     name - Name of the printer to look for.
//     node - BNode to set to the printer definition node.
//
// Returns:
//     B_OK if found, an error code otherwise.
// ---------------------------------------------------------------
status_t
PrintServerApp::FindPrinterNode(const char* name, BNode& node)
{
	// Find directory containing printer definitions
	BPath path;
	status_t rc = ::find_directory(B_USER_PRINTERS_DIRECTORY, &path, true, NULL);
	if (rc != B_OK)
		return rc;

	path.Append(name);
	return node.SetTo(path.Path());
}


bool 
PrintServerApp::OpenSettings(BFile& file, const char* name,
	bool forReading)
{
	BPath path;
	uint32 openMode = forReading ? B_READ_ONLY : B_CREATE_FILE | B_ERASE_FILE
		| B_WRITE_ONLY;
	return find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(name) == B_OK
		&& file.SetTo(path.Path(), openMode) == B_OK;
}


void 
PrintServerApp::LoadSettings() {
	BFile file;
	if (OpenSettings(file, kSettingsName, true)) {
		fSettings->Load(&file);
		fUseConfigWindow = fSettings->UseConfigWindow();
	}
}


void 
PrintServerApp::SaveSettings() {
	BFile file;
	if (OpenSettings(file, kSettingsName, false)) {
		fSettings->SetUseConfigWindow(fUseConfigWindow);
		fSettings->Save(&file);
	}
}
