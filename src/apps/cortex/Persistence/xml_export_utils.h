// xml_export_utils.h
// * PURPOSE
//   helper functions for writing XML representations of
//   C++ objects.
//
// * HISTORY
//   e.moon		5jul99		Begun

#ifndef __xml_export_utils_H__
#define __xml_export_utils_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// Writes the correct number of spaces to the given stream,
// so that text written after this call will start at the given
// column.
// Assumes that the given text string has already been written
// (with the level of indentation currently stored in the given
// context.)

inline ostream& pad_with_spaces(
	ostream&						str,
	const char*						text,
	ExportContext&			context,
	uint16							column=30) {

	int16 spaces = column - (strlen(text) + context.indentLevel());
	if(spaces < 0) spaces = 0;
	while(spaces--) str << ' ';
	return str;
}


// Writes the given key/value as an XML attribute (a newline
// is written first, to facilitate compact representation of
// elements with no attributes.)

template <class T>
void write_attr(
	const char* key,
	T value,
	ostream& stream,
	ExportContext& context) {
	
	stream << endl << context.indentString() << key;
	pad_with_spaces(stream, key, context) << " = '" << value << '\'';
}

__END_CORTEX_NAMESPACE

#endif /*__xml_export_utils_H__*/
