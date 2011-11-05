/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// RouteApp.cpp
// e.moon 14may99

#include "RouteApp.h"
#include "RouteWindow.h"
#include "DormantNodeWindow.h"
#include "MediaRoutingView.h"
#include "MediaNodePanel.h"

#include "RouteAppNodeManager.h"
#include "NodeRef.h"

#include "TipManager.h"

#include "AddOnHost.h"

#include "route_app_io.h"
#include "XML.h"
#include "MessageIO.h"
#include "NodeSetIOContext.h"

#include <Debug.h>
#include <OS.h>
#include <Roster.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Entry.h>

extern "C" void SetNewLeakChecking(bool);
extern "C" void SetMallocLeakChecking(bool);

using namespace std;

__USE_CORTEX_NAMESPACE

const char* const		RouteApp::s_settingsDirectory = "Cortex";
const char* const		RouteApp::s_settingsFile = "cortex_settings";

const char* const		RouteApp::s_appSignature = "application/x-vnd.Cortex.Route";

BMimeType						RouteApp::s_nodeSetType("text/x-vnd.Cortex.NodeSet");

const char* const		RouteApp::s_rootElement = "cortex_settings";
const char* const		RouteApp::s_mediaRoutingViewElement = "MediaRoutingView";
const char* const		RouteApp::s_routeWindowElement = "RouteWindow";

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

RouteApp::~RouteApp() {
//	PRINT((
//		"RouteApp::~RouteApp()\n"));

	ASSERT(manager);
	thread_id id = manager->Thread();
	manager->release();
	
//	PRINT((
//		"- waiting for manager to die\n"));
	if(id >= B_OK) {
		status_t err;
		while(wait_for_thread(id, &err) == B_INTERRUPTED) {
			PRINT((" * RouteApp::~RouteApp(): B_INTERRUPTED\n"));
		}
	}
//	PRINT((
//		"- RouteApp done.\n"));

	// [e.moon 6nov99] kill off the AddOnHost app, if any
	AddOnHost::Kill();
	
	if(m_settingsDocType)
		delete m_settingsDocType;
		
	if(m_nodeSetDocType)
		delete m_nodeSetDocType;
}

RouteApp::RouteApp() :
	BApplication(s_appSignature),
	manager(new RouteAppNodeManager(true)),
	routeWindow(0),
	m_settingsDocType(_createSettingsDocType()),
	m_nodeSetDocType(_createNodeSetDocType()),
	m_openPanel(B_OPEN_PANEL),
	m_savePanel(B_SAVE_PANEL) {
	
	// register MIME type(s)
	_InitMimeTypes();
	
	// create the window hierarchy
	RouteWindow*& r = const_cast<RouteWindow*&>(routeWindow);
	r = new RouteWindow(manager);
	
	// restore settings
	_readSettings();
	
	// fit windows to screen
	routeWindow->constrainToScreen();

	// show main window & palettes	
	routeWindow->Show();
}


bool
RouteApp::QuitRequested()
{
	// [e.moon 20oct99] make sure the main window is dead before quitting

	// store window positions & other settings
	// write settings file
	_writeSettings();

	routeWindow->_closePalettes();
	routeWindow->Lock();
	routeWindow->Quit();
	RouteWindow*& r = const_cast<RouteWindow*&>(routeWindow);
	r = 0;
		
	// clean up the TipManager [e.moon 19oct99]
	TipManager::QuitInstance();

	return true;
}

// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void RouteApp::MessageReceived(
	BMessage*									message) {
	
	status_t err;
	
	entry_ref ref;
	const char* name;
	
	switch(message->what) {
		
		case M_SHOW_OPEN_PANEL:
			m_openPanel.Show();
			break;

		case M_SHOW_SAVE_PANEL:
			m_savePanel.Show();
			break;

		case B_SAVE_REQUESTED: {
			err = message->FindRef("directory", &ref);
			if(err < B_OK)
				break;
			err = message->FindString("name", &name);
			if(err < B_OK)
				break;
			
			_writeSelectedNodeSet(&ref, name);

			m_savePanel.GetPanelDirectory(&ref);
			BEntry e(&ref);
			m_lastIODir.SetTo(&e);
			break;
		}
		
		default:
			_inherited::MessageReceived(message);
	}
}

// -------------------------------------------------------- //
// *** BApplication
// -------------------------------------------------------- //

void RouteApp::RefsReceived(
	BMessage*									message) {

	PRINT(("### RefsReceived\n"));

	status_t err;
	
	entry_ref ref;

	for(int32 n = 0; ; ++n) {	
		err = message->FindRef("refs", n, &ref);
		if(err < B_OK)
			break;
		
		_readNodeSet(&ref);
		
		m_openPanel.GetPanelDirectory(&ref);
		BEntry e(&ref);
		m_lastIODir.SetTo(&e);
	}
}

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// EXPORT

void RouteApp::xmlExportBegin(
	ExportContext&						context) const {
	context.beginElement(s_rootElement);
}
	
void RouteApp::xmlExportAttributes(
	ExportContext&						context) const {} //nyi: write version info +++++

// +++++
void RouteApp::xmlExportContent(
	ExportContext&						context) const {

	status_t err;
	context.beginContent();

	// export app settings
	{
		BMessage m;
		exportState(&m);
		MessageIO io(&m);
		err = context.writeObject(&io);
		ASSERT(err == B_OK);
	}
	
	if(routeWindow) {
		// export main routing window (frame/palette) settings
		context.beginElement(s_routeWindowElement);
		context.beginContent();
		BMessage m;
		if (routeWindow->Lock()) {
			routeWindow->exportState(&m);
			routeWindow->Unlock();
		}
		MessageIO io(&m);
		context.writeObject(&io);
		context.endElement();

		// export routing view (content) settings
		m.MakeEmpty();
		ASSERT(routeWindow->m_routingView);
		context.beginElement(s_mediaRoutingViewElement);
		context.beginContent();
		routeWindow->m_routingView->exportState(&m);
		context.writeObject(&io);
		context.endElement();
	}	
}
	
void RouteApp::xmlExportEnd(
	ExportContext&						context) const {
	context.endElement();
}

// IMPORT

void RouteApp::xmlImportBegin(
	ImportContext&						context) {

	m_readState = _READ_ROOT;
}

void RouteApp::xmlImportAttribute(
	const char*								key,
	const char*								value,
	ImportContext&						context) {} //nyi
	
void RouteApp::xmlImportContent(
	const char*								data,
	uint32										length,
	ImportContext&						context) {} //nyi
	
void RouteApp::xmlImportChild(
	IPersistent*							child,
	ImportContext&						context) {
	
	MessageIO* io = dynamic_cast<MessageIO*>(child);
	if(io) {
		ASSERT(io->message());
//		PRINT(("* RouteApp::xmlImportChild() [flat message]:\n"));
//		io->message()->PrintToStream();
		
		switch(m_readState) {
			case _READ_ROOT:
				importState(io->message());
				break;
			
			case _READ_ROUTE_WINDOW:
				ASSERT(routeWindow);
				routeWindow->importState(io->message());
				break;
				
			case _READ_MEDIA_ROUTING_VIEW:
				ASSERT(routeWindow);
				ASSERT(routeWindow->m_routingView);
				routeWindow->m_routingView->importState(io->message());
				break;

			default:
				PRINT(("! RouteApp::xmlImportChild(): unimplemented target\n"));
				break;
		}
	}
}
	
void RouteApp::xmlImportComplete(
	ImportContext&						context) {} //nyi

void RouteApp::xmlImportChildBegin(
	const char*								name,
	ImportContext&						context) {

	if(m_readState != _READ_ROOT) {
		context.reportError("RouteApp import: invalid nested element");
		return;
	}

	if(!strcmp(name, s_routeWindowElement)) {
		m_readState = _READ_ROUTE_WINDOW;
	}
	else if(!strcmp(name, s_mediaRoutingViewElement)) {
		m_readState = _READ_MEDIA_ROUTING_VIEW;
	}
	else {
		context.reportError("RouteApp import: unknown child element");
	}
}

void RouteApp::xmlImportChildComplete(
	const char*								name,
	ImportContext&						context) {

	if(m_readState == _READ_ROOT) {
		context.reportError("RouteApp import: garbled state");
		return;
	}
	m_readState = _READ_ROOT;
}

// -------------------------------------------------------- //
// *** IStateArchivable
// -------------------------------------------------------- //

status_t RouteApp::importState(
	const BMessage*						archive) {

	const char* last;
	if(archive->FindString("lastDir", &last) == B_OK) {
		m_lastIODir.SetTo(last);
		m_openPanel.SetPanelDirectory(last);
		m_savePanel.SetPanelDirectory(last);
	}
	
	return B_OK;
}

status_t RouteApp::exportState(
	BMessage*									archive) const {
	
	if(m_lastIODir.InitCheck() == B_OK)
		archive->AddString("lastDir", m_lastIODir.Path());
	
	return B_OK;
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

XML::DocumentType* RouteApp::_createSettingsDocType() {

	XML::DocumentType* docType = new XML::DocumentType(
		s_rootElement);
	MessageIO::AddTo(docType);
	
	return docType;
}

XML::DocumentType* RouteApp::_createNodeSetDocType() {

	XML::DocumentType* docType = new XML::DocumentType(
		_NODE_SET_ELEMENT);
	RouteAppNodeManager::AddTo(docType);
	
	return docType;
}

status_t RouteApp::_readSettings() {

	// figure path
	BPath path;
	status_t err = find_directory(
		B_USER_SETTINGS_DIRECTORY,
		&path);
	ASSERT(err == B_OK);
	
	path.Append(s_settingsDirectory);
	BEntry entry(path.Path());
	if(!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	path.Append(s_settingsFile);
	entry.SetTo(path.Path());
	if(!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	// open the settings file
	BFile file(&entry, B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return file.InitCheck();
	
	// read it:
	list<BString> errors;
	err = XML::Read(
		&file,
		this,
		m_settingsDocType,
		&errors);

	if(errors.size()) {
		fputs("!!! RouteApp::_readSettings():", stderr);
		for(list<BString>::iterator it = errors.begin();
			it != errors.end(); ++it)
			fputs((*it).String(), stderr);
	}
	return err;			
}

status_t RouteApp::_writeSettings() {
	// figure path, creating settings folder if necessary
	BPath path;
	status_t err = find_directory(
		B_USER_SETTINGS_DIRECTORY,
		&path);
	ASSERT(err == B_OK);

	BDirectory baseDirectory, settingsDirectory;
	
	err = baseDirectory.SetTo(path.Path());
	if(err < B_OK)
		return err;
	
	path.Append(s_settingsDirectory);

	BEntry folderEntry(path.Path());
	if(!folderEntry.Exists()) {
		// create folder
		err = baseDirectory.CreateDirectory(s_settingsDirectory, &settingsDirectory);
		ASSERT(err == B_OK);
	}
	else
		settingsDirectory.SetTo(&folderEntry);
		
	// open/clobber file
	BFile file(
		&settingsDirectory,
		s_settingsFile,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	err = file.InitCheck();
	if(err < B_OK)
		return err;

	// write document header
	const char* header = "<?xml version=\"1.0\"?>\n";
	file.Write((const void*)header, strlen(header));
	
	// write content
	BString errorText;
	err = XML::Write(
		&file,
		this,
		&errorText);
	
	if(err < B_OK) {
		fprintf(stderr,
			"!!! RouteApp::_writeSettings() failed: %s\n",
			errorText.String());
	}

	return err;
}

// -------------------------------------------------------- //

class _RouteAppImportContext :
	public	ImportContext,
	public	NodeSetIOContext {

public:
	_RouteAppImportContext(
		list<BString>&							errors,
		MediaRoutingView*						routingView) :
		ImportContext(errors),
		m_routingView(routingView) {}

public:													// *** hooks
	virtual void importUIState(
		const BMessage*							archive) {
	
		PRINT((
			"### importUIState\n"));	
		
		if(m_routingView) {
//			m_routingView->LockLooper();
			m_routingView->DeselectAll();
			status_t err = m_routingView->importStateFor(
				this,
				archive);
			if(err < B_OK) {
				PRINT((
					"!!! _RouteAppImportContext::importStateFor() failed:\n"
					"    %s\n", strerror(err)));
			}
			m_routingView->Invalidate(); // +++++ not particularly clean
//			m_routingView->UnlockLooper();
		}
	}
	
	MediaRoutingView*							m_routingView;
};

status_t RouteApp::_readNodeSet(
	entry_ref*								ref) {

	BFile file(ref, B_READ_ONLY);
	status_t err = file.InitCheck();
	if(err < B_OK)
		return err;

	routeWindow->Lock();
	
	list<BString> errors;

	err = XML::Read(
		&file,
		manager,
		m_nodeSetDocType,
		new _RouteAppImportContext(errors, routeWindow->m_routingView));

	routeWindow->Unlock();

	if(errors.size()) {
		fputs("!!! RouteApp::_readNodeSet():", stderr);
		for(list<BString>::iterator it = errors.begin();
			it != errors.end(); ++it)
			fputs((*it).String(), stderr);
	}
	return err;			
}

// -------------------------------------------------------- //

class _RouteAppExportContext :
	public	ExportContext,
	public	NodeSetIOContext {

public:
	_RouteAppExportContext(
		MediaRoutingView*						routingView) :
		m_routingView(routingView) {}

public:													// *** hooks
	virtual void exportUIState(
		BMessage*										archive) {
	
		PRINT((
			"### exportUIState\n"));	

		if(m_routingView) {
			m_routingView->LockLooper();
			m_routingView->exportStateFor(
				this,
				archive);
			m_routingView->UnlockLooper();
		}
	}

	MediaRoutingView*							m_routingView;
};

status_t RouteApp::_writeSelectedNodeSet(
	entry_ref*								dirRef,
	const char*								filename) {
	
	status_t err;


	// sanity-check & fetch the selection
	routeWindow->Lock();
	
	MediaRoutingView* v = routeWindow->m_routingView;
	ASSERT(v);

	if(
		v->CountSelectedItems() < 0 ||
		v->SelectedType() != DiagramItem::M_BOX) {
		PRINT((
			"!!! RouteApp::_writeSelectedNodeSet():\n"
			"    Invalid selection!\n"));
			
		routeWindow->Unlock();
		return B_NOT_ALLOWED;
	}

	_RouteAppExportContext context(v);
	
	for(uint32 i = 0; i < v->CountSelectedItems(); ++i) {
		MediaNodePanel* panel = dynamic_cast<MediaNodePanel*>(v->SelectedItemAt(i));
		if(!panel)
			continue;
		err = context.addNode(panel->ref->id());
		if(err < B_OK) {
			PRINT((	
				"!!! context.addNode() failed: '%s\n", strerror(err)));
		}
	}
	routeWindow->Unlock();

	// open/clobber file
	BDirectory dir(dirRef);
	err = dir.InitCheck();
	if(err < B_OK)
		return err;

	BFile file(
		&dir,
		filename,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	err = file.InitCheck();
	if(err < B_OK)
		return err;
	
	// write document header
	const char* header = "<?xml version=\"1.0\"?>\n";
	file.Write((const void*)header, strlen(header));
	
	// export nodes
	context.stream = &file;
	err = context.writeObject(manager);
	if(err < B_OK) {
		PRINT((
			"!!! RouteApp::_writeSelectedNodeSet(): error:\n"
			"    %s\n", context.errorText()));

		// +++++ delete the malformed file
		
	}


	// write MIME type
	BNodeInfo* fileInfo = new BNodeInfo(&file);
	fileInfo->SetType(s_nodeSetType.Type());
	fileInfo->SetPreferredApp(s_appSignature);
	delete fileInfo;	
	
	return B_OK;
}

/*static*/
status_t RouteApp::_InitMimeTypes() {
	
	status_t err;
	
	ASSERT(s_nodeSetType.IsValid());
	
	if(!s_nodeSetType.IsInstalled()) {
		err = s_nodeSetType.Install();
		if(err < B_OK) {
			PRINT((
				"!!! RouteApp::_InitMimeTypes(): Install():\n"
				"    %s\n", strerror(err)));
			return err;
		}
		
		err = s_nodeSetType.SetPreferredApp(s_appSignature);
		if(err < B_OK) {
			PRINT((
				"!!! RouteApp::_InitMimeTypes(): SetPreferredApp():\n"
				"    %s\n", strerror(err)));
			return err;
		}
	}
	
	return B_OK;
}

// -------------------------------------------------------- //
// main() stub
// -------------------------------------------------------- //

int main(int argc, char** argv) {
//	SetNewLeakChecking(true);
//	SetMallocLeakChecking(true);
	
	RouteApp app;
	app.Run();

	return 0;
}

// END -- RouteApp.cpp --
