/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
//
//
// TODO:
//	 Handle spooled jobs at startup
//	 Handle printer status (asked to print a spooled job but printer busy)
//   
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "PrintServerApp.h"

#include "BeUtils.h"
#include "Printer.h"
#include "pr_server.h"

	// BeOS API
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Directory.h>
#include <PrintJob.h>
#include <NodeInfo.h>
#include <String.h>
#include <Roster.h>
#include <image.h>
#include <Alert.h>
#include <Path.h>
#include <File.h>
#include <Mime.h>

	// ANSI C
#include <stdio.h>	//for printf
#include <unistd.h> //for unlink

// ---------------------------------------------------------------
typedef struct _printer_data {
	char defaultPrinterName[256];
} printer_data_t;

typedef BMessage* (*config_func_t)(BNode*, const BMessage*);
typedef BMessage* (*take_job_func_t)(BFile*, BNode*, const BMessage*);
typedef char* (*add_printer_func_t)(const char* printer_name);

/**
 * Main entry point of print_server.
 *
 * @returns B_OK if application was started, or an errorcode if
 *          application failed to start.
 *
 * @see your favorite C/C++ textbook :P
 */

int
main()
{
		// Create our application object
	status_t rc = B_OK;
	new PrintServerApp(&rc);
	
		// If all went fine, let's start it
	if (rc == B_OK) {
		be_app->Run();
	}
	
	delete be_app;
	
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
	fSelectedIconMini(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1), B_CMAP8),
	fSelectedIconLarge(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8)
{
		// If our superclass initialized ok
	if (*err == B_OK) {
			// let us try as well
		SetupPrinterList();
		RetrieveDefaultPrinter();
		
			// Cache icons for selected printer
		BMimeType type(PRNT_SIGNATURE_TYPE);
		type.GetIcon(&fSelectedIconMini, B_MINI_ICON);
		type.GetIcon(&fSelectedIconLarge, B_LARGE_ICON);
	}
}

bool PrintServerApp::QuitRequested()
{
	bool rc = Inherited::QuitRequested();
	if (rc) {		
			// Find directory containing printer definition nodes
		BPath path;
		if (::find_directory(B_USER_PRINTERS_DIRECTORY, &path) == B_OK) {
			BEntry entry(path.Path());
			node_ref nref;
			if (entry.InitCheck() == B_OK &&
				entry.GetNodeRef(&nref) == B_OK) {
					// Stop watching the printer directory
				::watch_node(&nref, B_STOP_WATCHING, be_app_messenger);
			}
		}
	}
	
	return rc;
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
status_t PrintServerApp::SetupPrinterList()
{
	BEntry entry;
	status_t rc;
	BPath path;
	
		// Find directory containing printer definition nodes
	if ((rc=::find_directory(B_USER_PRINTERS_DIRECTORY, &path)) == B_OK) {
		BDirectory dir(path.Path());
		
			// Can we reach the directory?
		if ((rc=dir.InitCheck()) == B_OK) {
				// Yes, so loop over it's entries
			while((rc=dir.GetNextEntry(&entry)) == B_OK) {
					// If the entry is a directory
				if (entry.IsDirectory()) {
						// Check it's Mime type for a spool director
					BNode node(&entry);
					BNodeInfo info(&node);
					char buffer[256];
					
					if (info.GetType(buffer) == B_OK &&
						strcmp(buffer, PSRV_PRINTER_FILETYPE) == 0) {
							// Yes, it is a printer definition node
						new Printer(&node);
					}
				}
			}
			
				// If we scanned all entries successfully
			node_ref nref;
			if (rc == B_ENTRY_NOT_FOUND &&
				(rc=dir.GetNodeRef(&nref)) == B_OK) {
					// Get node to watch
				rc = ::watch_node(&nref, B_WATCH_DIRECTORY, be_app_messenger);
			}
		}
	}
	
	return rc;
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
			printf("PrintServerApp::MessageReceived(): ");
			msg->PrintToStream();			
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
	const char* connection, const char* transportName, const char* transportPath)
{
	status_t rc;
	
		// Find directory containing printer definitions
	BPath path;
	if ((rc=::find_directory(B_USER_PRINTERS_DIRECTORY,&path,true,NULL)) == B_OK) {

			// Create our printer definition/spool directory
		BDirectory printersDir(path.Path());
		BDirectory printer;
		if ((rc=printersDir.CreateDirectory(printerName,&printer)) == B_OK) {
				// Set its type to a printer
			BNodeInfo info(&printer);
			info.SetType(PSRV_PRINTER_FILETYPE);
			
				// Store the settings in its attributes
			printer.WriteAttr(PSRV_PRINTER_ATTR_PRT_NAME, B_STRING_TYPE, 0, printerName, 	::strlen(printerName)+1);
			printer.WriteAttr(PSRV_PRINTER_ATTR_DRV_NAME, B_STRING_TYPE, 0, driverName,	::strlen(driverName)+1);
			printer.WriteAttr(PSRV_PRINTER_ATTR_STATE, B_STRING_TYPE, 0, "free", 		::strlen("free")+1);
			printer.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT, B_STRING_TYPE, 0, transportName,::strlen(transportName)+1);
			printer.WriteAttr(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0, transportPath,::strlen(transportPath)+1);
			printer.WriteAttr(PSRV_PRINTER_ATTR_CNX, B_STRING_TYPE, 0, connection,	::strlen(connection)+1);
			
				// Find printer driver
			if (FindPrinterDriver(driverName, path) == B_OK) {
					// Try and load the addon
				image_id id = ::load_add_on(path.Path());
				if (id > 0) {
						// Addon was loaded, so try and get the add_printer symbol
					add_printer_func_t func;
					if (get_image_symbol(id, "add_printer", B_SYMBOL_TYPE_TEXT, (void**)&func) == B_OK) {
							// call the function and check its result
						if ((*func)(printerName) == NULL) {
							BEntry entry;
							if (printer.GetEntry(&entry) == B_OK &&	entry.GetPath(&path) == B_OK) {
									// Delete the printer if function failed
								::rmdir(path.Path());
							}
						}
					}
					
					::unload_add_on(id);
				}
			}
		}
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
	if (fDefaultPrinter != NULL &&
		FindPrinterNode(fDefaultPrinter->Name(), node) == B_OK) {
			// and remove the custom icon
		BNodeInfo info(&node);
		info.SetIcon(NULL, B_MINI_ICON);
		info.SetIcon(NULL, B_LARGE_ICON);
	}

		// Find the node for the new default printer
	if ((rc=FindPrinterNode(printerName, node)) == B_OK) {
			// and add the custom icon
		BNodeInfo info(&node);
		info.SetIcon(&fSelectedIconMini, B_MINI_ICON);
		info.SetIcon(&fSelectedIconLarge, B_LARGE_ICON);
	}
	
	fDefaultPrinter = Printer::Find(printerName);
	StoreDefaultPrinter(); // update our pref file
	be_roster->Broadcast(new BMessage(B_PRINTER_CHANGED));
	
	return rc;
}

// ---------------------------------------------------------------
// HandleSpooledJob(const char* jobname, const char* filepath)
//
// Handles calling the printer driver for printing a spooled job.
//
// Parameters:
//      msg - A copy of the message received, will be accessible
//            to the printer driver for reading job config 
//            details.
// Returns:
//      BMessage containing new job data, or NULL pointer if not
//		successfull.
// ---------------------------------------------------------------
status_t
PrintServerApp::HandleSpooledJob(const char* jobname, const char* filepath)
{
	BFile spoolFile(filepath, B_READ_ONLY);
	print_file_header header;
	BString driverName;
	BMessage settings;
	Printer* printer;
	status_t rc;
	BPath path;
	
		// Open the spool file
	if ((rc=spoolFile.InitCheck()) == B_OK) {
			// Read header from filestream
		spoolFile.Seek(0, SEEK_SET);
		spoolFile.Read(&header, sizeof(header));

			// Get the printer settings
		settings.Unflatten(&spoolFile);

			// Get name of printer from settings
		const char* printerName;
		if ((rc=settings.FindString("current_printer", &printerName)) == B_OK) {
				// Find printer definition for this printer
			if ((printer=Printer::Find(printerName)) != NULL) {
					// and tell the printer to print the spooled job
				printer->PrintSpooledJob(&spoolFile, settings);
			}
		}

			// Remove spool file if printing was successfull.
		if (rc == B_OK) {
			::unlink(filepath);
		}
	}

	return rc;
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
	printer_data_t prefs;
	status_t rc = B_OK;
	BPath path;

	if ((rc=find_directory(B_USER_SETTINGS_DIRECTORY, &path, true)) == B_OK) {
		BFile file;
		
		path.Append("printer_data");

		if ((rc=file.SetTo(path.Path(), B_READ_ONLY)) == B_OK) {
			file.ReadAt(0, &prefs, sizeof(prefs));
			fDefaultPrinter = Printer::Find(prefs.defaultPrinterName);
		}
	}
	
	return rc;
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
	printer_data_t prefs;
	status_t rc = B_OK;
	BPath path;

	if ((rc=find_directory(B_USER_SETTINGS_DIRECTORY, &path, true)) == B_OK) {
		BFile file;
		
		path.Append("printer_data");

		if ((rc=file.SetTo(path.Path(), B_WRITE_ONLY)) == B_OK) {

			if (fDefaultPrinter != NULL)
				::strcpy(prefs.defaultPrinterName, fDefaultPrinter->Name());
			else
				prefs.defaultPrinterName[0] = '\0';

			file.WriteAt(0, &prefs, sizeof(prefs));
		}
	}
	
	return rc;
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
	status_t rc;
	
		// Find directory containing printer definitions
	BPath path;
	if ((rc=::find_directory(B_USER_PRINTERS_DIRECTORY,&path,true,NULL)) == B_OK)
	{
		path.Append(name);
		rc = node.SetTo(path.Path());
	}
	
	return rc;
}

// ---------------------------------------------------------------
// FindPrinterDriver(const char* name, BPath& outPath)
//
// Finds the path to a specific printer driver. It searches all 3
// places add-ons can be stored: the user's private directory, the
// directory common to all users, and the system directory, in that
// order.
//
// Parameters:
//     name - Name of the printer driver to look for.
//     outPath - BPath to store the path to the driver in.
//
// Returns:
//      B_OK if the driver was found, otherwise an error code.
// ---------------------------------------------------------------
status_t
PrintServerApp::FindPrinterDriver(const char* name, BPath& outPath)
{
		// try to locate the driver
	if (::TestForAddonExistence(name, B_USER_ADDONS_DIRECTORY, "Print", outPath) != B_OK)
		if (::TestForAddonExistence(name, B_COMMON_ADDONS_DIRECTORY, "Print", outPath) != B_OK)
			return ::TestForAddonExistence(name, B_BEOS_ADDONS_DIRECTORY, "Print", outPath);

	return B_OK;
}
