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


// RouteApp.h
// e.moon 14may99
//
// PURPOSE
//   Application class for route/1.
//
// HISTORY
//   14may99		e.moon		Created from 'routeApp.cpp'

#ifndef __ROUTEAPP_H__
#define __ROUTEAPP_H__

#include <Application.h>
#include <FilePanel.h>
#include <Mime.h>
#include <Path.h>

#include "XML.h"
#include "IStateArchivable.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class RouteWindow;
class RouteAppNodeManager;

class RouteApp :
	public		BApplication,
	public		IPersistent,
	public		IStateArchivable {

	typedef		BApplication _inherited;

public:															// members
	RouteAppNodeManager* const				manager;
	RouteWindow* const								routeWindow;

	static BMimeType									s_nodeSetType;
	
public:												// messages
	enum message_t {	
		// [e.moon 2dec99]
		M_SHOW_OPEN_PANEL					=RouteApp_message_base,
		M_SHOW_SAVE_PANEL
	};
	
public:												// ctor/dtor
	virtual ~RouteApp();
	RouteApp();

	bool QuitRequested();

public:												// *** BHandler
	virtual void MessageReceived(
		BMessage*									message);

public:												// *** BApplication
	virtual void RefsReceived(
		BMessage*									message);

public:												// *** IPersistent

	// EXPORT
	
	void xmlExportBegin(
		ExportContext&						context) const;
		
	void xmlExportAttributes(
		ExportContext&						context) const;

	void xmlExportContent(
		ExportContext&						context) const;
		
	void xmlExportEnd(
		ExportContext&						context) const;
	
	// IMPORT
	
	void xmlImportBegin(
		ImportContext&						context);
	
	void xmlImportAttribute(
		const char*								key,
		const char*								value,
		ImportContext&						context);
		
	void xmlImportContent(
		const char*								data,
		uint32										length,
		ImportContext&						context);
		
	void xmlImportChild(
		IPersistent*							child,
		ImportContext&						context);
		
	void xmlImportComplete(
		ImportContext&						context);

	void xmlImportChildBegin(
		const char*								name,
		ImportContext&						context);

	void xmlImportChildComplete(
		const char*								name,
		ImportContext&						context);

public:												// *** IStateArchivable

	status_t importState(
		const BMessage*						archive);
	
	status_t exportState(
		BMessage*									archive) const;

private:
	static const char* const		s_appSignature;
	
	XML::DocumentType*					m_settingsDocType;
	XML::DocumentType*					m_nodeSetDocType;
	
	enum {
		_READ_ROOT,
		_READ_ROUTE_WINDOW,
		_READ_MEDIA_ROUTING_VIEW
	}														m_readState;
	
	BPath												m_lastIODir;
	BFilePanel									m_openPanel;
	BFilePanel									m_savePanel;
	
private:
	XML::DocumentType* _createSettingsDocType();
	XML::DocumentType* _createNodeSetDocType();
	status_t _readSettings();
	status_t _writeSettings();
	
	status_t _readNodeSet(
		entry_ref*								ref);
	status_t _writeSelectedNodeSet(
		entry_ref*								dir,
		const char*								filename);
		
	static status_t _InitMimeTypes();

	static const char* const		s_settingsDirectory;
	static const char* const		s_settingsFile;
	
	static const char* const		s_rootElement;
	static const char* const		s_mediaRoutingViewElement;
	static const char* const		s_routeWindowElement;
};

__END_CORTEX_NAMESPACE
#endif /* __ROUTEAPP_H__ */
