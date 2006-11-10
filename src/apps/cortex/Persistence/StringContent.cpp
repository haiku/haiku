// StringContent.cpp

#include "StringContent.h"
#include "ImportContext.h"

#include <cctype>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// EXPORT [not implemented]
// -------------------------------------------------------- //

void StringContent::xmlExportBegin(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}	
void StringContent::xmlExportAttributes(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}
void StringContent::xmlExportContent(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}
void StringContent::xmlExportEnd(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}

// -------------------------------------------------------- //
// IMPORT	
// -------------------------------------------------------- //

void StringContent::xmlImportBegin(
	ImportContext&								context) {}

void StringContent::xmlImportAttribute(
	const char*										key,
	const char*										value,
	ImportContext&								context) {}
		
void StringContent::xmlImportContent(
	const char*										data,
	uint32												length,
	ImportContext&								context) {
	content.Append(data, length);
}

void StringContent::xmlImportChild(
	IPersistent*									child,
	ImportContext&								context) {
	context.reportError("StringContent: child not expected");
}
	
void StringContent::xmlImportComplete(
	ImportContext&								context) {

	// chomp leading/trailing whitespace
	if(content.Length() == 0)
		return;

	int32 last = 0;
	for(; last < content.Length() && isspace(content[last]); ++last) {}
	if(last > 0)
		content.Remove(0, last);

	last = content.Length() - 1;
	int32 from = last;
	for(; from > 0 && isspace(content[from]); --from) {}
	if(from < last)
		content.Remove(from+1, last-from);
}

// END -- StringContent.cpp --

