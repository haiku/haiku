// FlatMessageIO.h
// * PURPOSE
//   Efficient export/import of BMessages to and from
//   XML using the Cortex persistence library.
//   Messages are stored in flattened form.
// * HISTORY
//   e.moon		6jul99		Begun.
//
// Example:
//
// <flat-BMessage
//   encoding = 'base64'>
//   nKJNFBlkn3lknbxlkfnbLKN/lknlknlDSLKn3lkn3l2k35234ljk234
//   lkdsg23823nlknsdlkbNDSLKBNlkn3lk23n4kl23n423lknLKENL+==
// </flat-BMessage>

#ifndef __FlatMessageIO_H__
#define __FlatMessageIO_H__

#include <Message.h>
#include <String.h>
#include "XML.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class FlatMessageIO :
	public		IPersistent {

public:					// *** ctor/dtor/accessor
	virtual ~FlatMessageIO();

	FlatMessageIO();
	
	// When given a message to export, this object does NOT take
	// responsibility for deleting it.  It will, however, handle
	// deletion of an imported BMessage.  
	
	FlatMessageIO(const BMessage* message);
	void setMessage(BMessage* message);	
	
	// Returns 0 if no message has been set, and if no message has
	// been imported.
	
	const BMessage* message() const { return m_message; }
	
	// Returns true if the message will be automatically deleted.
	
	bool ownsMessage() const { return m_ownMessage; }

public:					// *** static setup method
	// call this method to install hooks for the tags needed by
	// FlatMessageIO into the given document type
	static void AddTo(XML::DocumentType* pDocType);
	
public:					// *** XML formatting
	static const char* const			s_element;
	static const uint16					s_encodeToMax			= 72;
	static const uint16					s_encodeToMin			= 24;

public:					// *** IPersistent impl.

//	virtual void xmlExport(
//		ostream& 					stream,
//		ExportContext&		context) const;

	// EXPORT:
	
	void xmlExportBegin(
		ExportContext& context) const;
		
	void xmlExportAttributes(
		ExportContext& context) const;
	
	void xmlExportContent(
		ExportContext& context) const;
	
	void xmlExportEnd(
		ExportContext& context) const;


	// IMPORT:

	virtual void xmlImportBegin(
		ImportContext&		context);

	virtual void xmlImportAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context);
	
	virtual void xmlImportContent(
		const char*					data,
		uint32						length,
		ImportContext&		context);
	
	virtual void xmlImportChild(
		IPersistent*			child,
		ImportContext&		context);
			
	virtual void xmlImportComplete(
		ImportContext&		context);
		
private:					// *** members
	bool						m_ownMessage;
	BMessage*			m_message;

	// encoded data is cached here during the import process	
	BString				m_base64Data;
};

__END_CORTEX_NAMESPACE

#endif /*__FlatMessageIO_H__*/
