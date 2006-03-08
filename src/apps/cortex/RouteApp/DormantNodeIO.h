// DormantNodeIO.h
// * PURPOSE
//   Manage the import and export of a user-instantiated
//   media node descriptor.
//
// * HISTORY
//   e.moon		8dec99		Begun

#ifndef __DormantNodeIO_H__
#define __DormantNodeIO_H__

#include "NodeRef.h"
#include "XML.h"

#include <String.h>
#include <Entry.h>

class dormant_node_info;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeManager;

class DormantNodeIO :
	public		IPersistent {

public:											// *** ctor/dtor
	virtual ~DormantNodeIO();
	
	DormantNodeIO();
	DormantNodeIO(
		NodeRef*								ref,
		const char*							nodeKey);

	bool exportValid() const { return m_exportValid; }
	
	const char* nodeKey() const { return m_nodeKey.String(); }

public:											// *** operations

	// call when object imported to create the described node
	status_t instantiate(
		NodeManager*						manager,
		NodeRef**								outRef);

public:											// *** document-type setup
	static void AddTo(
		XML::DocumentType*			docType);

public:											// *** IPersistent

	// EXPORT:
	
	void xmlExportBegin(
		ExportContext&						context) const;
		
	void xmlExportAttributes(
		ExportContext&						context) const;
	
	void xmlExportContent(
		ExportContext&						context) const;
	
	void xmlExportEnd(
		ExportContext&						context) const;

	// IMPORT:

	virtual void xmlImportBegin(
		ImportContext&						context);

	virtual void xmlImportAttribute(
		const char*								key,
		const char*								value,
		ImportContext&						context);
	
	virtual void xmlImportContent(
		const char*								data,
		uint32										length,
		ImportContext&						context);
	
	virtual void xmlImportChild(
		IPersistent*							child,
		ImportContext&						context);
			
	virtual void xmlImportComplete(
		ImportContext&						context);
		
private:											// *** implementation
	// imported data
	BString										m_nodeKey;

	BString										m_dormantName;
	int64											m_kinds;
	int32											m_flavorID;
	
	int32											m_flags;
	int32											m_runMode;
	bigtime_t									m_recordingDelay;
	bool											m_cycle;

	BEntry										m_entry;

	bool											m_exportValid;
	
	status_t _matchDormantNode(
		dormant_node_info*			outInfo);
};

__END_CORTEX_NAMESPACE
#endif /*__DormantNodeIO_H__*/