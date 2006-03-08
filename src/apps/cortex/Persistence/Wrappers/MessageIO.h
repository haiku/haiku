// MessageIO.h
// * PURPOSE
//   Export/import of BMessages to and from
//   XML using the Cortex persistence library.
//   Messages are stored in a user-readable form.
//
//   TO DO +++++
//   - sanity-check string values (filter/escape single quotes)
//
// * HISTORY
//   e.moon		1dec99		Begun.

#ifndef __MessageIO_H__
#define __MessageIO_H__

#include <Message.h>
#include <String.h>
#include "XML.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MessageIO :
	public		IPersistent {

public:												// *** ctor/dtor/accessor
	virtual ~MessageIO();

	MessageIO(); //nyi
	
	// When given a message to export, this object does NOT take
	// responsibility for deleting it.  It will, however, handle
	// deletion of an imported BMessage.  
	
	MessageIO(
		const BMessage*						message);
	void setMessage(
		BMessage*									message);
	
	// Returns 0 if no message has been set, and if no message has
	// been imported.
	
	const BMessage* message() const { return m_message; }
	
	// Returns true if the message will be automatically deleted.
	
	bool ownsMessage() const { return m_ownMessage; }

public:												// *** static setup method
	// call this method to install hooks for the tags needed by
	// MessageIO into the given document type
	static void AddTo(
		XML::DocumentType*				docType);
	
public:												// *** XML formatting
	static const char* const		s_element;

public:												// *** IPersistent impl.

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
		
	virtual void xmlImportChildBegin(
		const char*								name,
		ImportContext&						context);

	virtual void xmlImportChildAttribute(
		const char*								key,
		const char*								value,
		ImportContext&						context);
	
	virtual void xmlImportChildContent(
		const char*								data,
		uint32										length,
		ImportContext&						context);

	virtual void xmlImportChildComplete(
		const char*								name,
		ImportContext&						context);

private:											// *** members
	bool												m_ownMessage;
	BMessage*										m_message;
	
	// name of the message (if used to import a nested BMessage)
	BString											m_name;
	
	// current field
	BString											m_fieldName;
	BString											m_fieldData;
	
	bool _isValidMessageElement(
		const char*								element) const;

	status_t _importField(
		BMessage*									message,
		const char*								element,
		const char*								name,
		const char*								data);
		
	status_t _exportField(
		ExportContext&						context,
		BMessage*									message,
		type_code									type,
		const char*								name,
		int32											index) const;
};

__END_CORTEX_NAMESPACE

#endif /*__MessageIO_H__*/