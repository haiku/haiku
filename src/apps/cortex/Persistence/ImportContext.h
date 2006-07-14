// ImportContext.h
// * PURPOSE
//   Describe the state of a deserialization ('load') operation.
//   The 'save' equivalent is ExportContext.
//
// * HISTORY
//   e.moon		29jun99		Begun

#ifndef __ImportContext_H__
#define __ImportContext_H__

#include <list>
#include <utility>
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IPersistent;

class ImportContext {
	friend class Importer;

public:													// *** types
	enum state_t {
		PARSING,
		COMPLETE,
		ABORT
	};

public:													// *** ctor/dtor
	virtual ~ImportContext();
	ImportContext(
		std::list<BString>&						errors);

public:													// *** accessors

	// fetch the current element (tag)
	// (returns 0 if the stack is empty)
	const char* element() const;
	
	// fetch the current element's parent
	// (returns 0 if the stack is empty or the current element is top-level)
	const char* parentElement() const;

	std::list<BString>& errors() const;
	const state_t state() const;

public:													// *** error-reporting operations

	// register a warning to be returned once the deserialization
	// process is complete.
	void reportWarning(
		const char*									text);
		
	// register a fatal error; halts the deserialization process
	// as soon as possible.
	void reportError(
		const char*									text);

protected:											// *** internal operations

	void reset();

private:												// *** members
	state_t												m_state;
	std::list<BString>&							m_errors;
	
	std::list<BString>								m_elementStack;
	typedef std::pair<const char*, IPersistent*> object_entry;
	std::list<object_entry>						m_objectStack;
	
	void*													m_pParser;
};
__END_CORTEX_NAMESPACE
#endif /*__ImportContext_H__*/
