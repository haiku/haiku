// StringContent.h
// * PURPOSE
//   Implements IPersistent to store element content in
//   a BString and automatically strip leading/trailing
//   whitespace.  Doesn't handle child elements; export
//   is not implemented (triggers an error).
//
// * HISTORY
//   e.moon			7dec99		Begun

#ifndef __StringContent_H__
#define __StringContent_H__

#include <MediaDefs.h>

#include "XML.h"
#include "cortex_defs.h"

__BEGIN_CORTEX_NAMESPACE

class StringContent :
	public	IPersistent {
	
public:														// content access
	BString													content;

public:														// *** dtor, default ctors
	virtual ~StringContent() {}
	StringContent() {}
	StringContent(
		const char*										c) : content(c) {}

public:														// *** IPersistent

	// EXPORT
	
	virtual void xmlExportBegin(
		ExportContext&								context) const;
		
	virtual void xmlExportAttributes(
		ExportContext&								context) const;

	virtual void xmlExportContent(
		ExportContext&								context) const;
	
	virtual void xmlExportEnd(
		ExportContext&								context) const;

	// IMPORT	
	
	virtual void xmlImportBegin(
		ImportContext&								context);
	
	virtual void xmlImportAttribute(
		const char*										key,
		const char*										value,
		ImportContext&								context);
			
	virtual void xmlImportContent(
		const char*										data,
		uint32												length,
		ImportContext&								context);
	
	virtual void xmlImportChild(
		IPersistent*									child,
		ImportContext&								context);
		
	virtual void xmlImportComplete(
		ImportContext&								context);
};

__END_CORTEX_NAMESPACE
#endif /*__StringContent_H__ */
