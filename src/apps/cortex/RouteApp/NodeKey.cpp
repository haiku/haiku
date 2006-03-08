// NodeKey.cpp

#include "NodeKey.h"
#include "ImportContext.h"

#include <cctype>

__USE_CORTEX_NAMESPACE

const char* const NodeKey::s_element = "node_key";

// -------------------------------------------------------- //
// EXPORT
// -------------------------------------------------------- //

void NodeKey::xmlExportBegin(
	ExportContext&								context) const {

	context.beginElement(s_element);
}
	
void NodeKey::xmlExportAttributes(
	ExportContext&								context) const {}

void NodeKey::xmlExportContent(
	ExportContext&								context) const {
	context.beginContent();
	context.writeString(content);
}

void NodeKey::xmlExportEnd(
	ExportContext&								context) const {
	context.endElement();
}

// END -- NodeKey.cpp --