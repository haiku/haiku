// ImportContext.cpp
// e.moon 1jul99

#include "ImportContext.h"
#include "xmlparse.h"

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ImportContext::~ImportContext() {}
ImportContext::ImportContext(list<BString>& errors) :
	m_state(PARSING),
	m_errors(errors),
	m_pParser(0) {}

// -------------------------------------------------------- //
// accessors
// -------------------------------------------------------- //

// fetch the current element (tag)
// (returns 0 if the stack is empty)
const char* ImportContext::element() const {
	return (m_elementStack.size()) ?
		m_elementStack.back().String() :
		0;
}

const char* ImportContext::parentElement() const {
	if(m_elementStack.size() < 2)
		return 0;
	list<BString>::const_reverse_iterator it = m_elementStack.rbegin();
	++it;
	return (*it).String();
}

list<BString>& ImportContext::errors() const {
	return m_errors;
}
const ImportContext::state_t ImportContext::state() const {
	return m_state;
}

// -------------------------------------------------------- //
// error-reporting operations
// -------------------------------------------------------- //

// register a warning to be returned once the deserialization
// process is complete.
void ImportContext::reportWarning(
	const char*			pText) {

	XML_Parser p = (XML_Parser)m_pParser;
	
	BString err = "Warning: ";
	err << pText;
	if(p) {
		err << "\n         (line " <<
			(uint32)XML_GetCurrentLineNumber(p) << ", column " <<
			(uint32)XML_GetCurrentColumnNumber(p) << ", element '" <<
			(element() ? element() : "(none)") << "')\n";
	} else
		err << "\n";
	m_errors.push_back(err);
}
		
// register a fatal error; halts the deserialization process
// as soon as possible.
void ImportContext::reportError(
	const char*			pText) {

	XML_Parser p = (XML_Parser)m_pParser;

	BString err = "FATAL ERROR: ";
	err << pText;
	if(p) {
		err << "\n             (line " <<
			(uint32)XML_GetCurrentLineNumber(p) << ", column " <<
			(uint32)XML_GetCurrentColumnNumber(p) << ", element '" <<
			(element() ? element() : "(none)") << "')\n";
	} else
		err << "\n";
	m_errors.push_back(err);
	
	m_state = ABORT;
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

void ImportContext::reset() {
	m_state = PARSING;
	m_elementStack.clear();
	// +++++ potential for memory leaks; reset() is currently
	//       only to be called after an identify cycle, during
	//       which no objects are created anyway, but this still
	//       gives me the shivers...
	m_objectStack.clear();
}

// END -- ImportContext.cpp --
