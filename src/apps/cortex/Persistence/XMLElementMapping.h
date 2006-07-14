// XMLElementMapping.h
// * PURPOSE
//   A simple class (template implementing a non-template
//   base interface) to encapsulate the operation:
//   "make an object for this XML element."
//
// * HISTORY
//   e.moon		04oct99		Begun.

#ifndef __XMLElementMapping_H__
#define __XMLElementMapping_H__

#include <functional>
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IPersistent;

// The base class:

class XMLElementMapping {
public:													// *** data
	const BString									element;
	
public:													// *** interface
	virtual ~XMLElementMapping() {}
	XMLElementMapping(
		const char*									_element) :
		element(_element) {}
	
	virtual IPersistent* create() const =0;	
};

// The template:

template <class T>
class Mapping :
	public	XMLElementMapping {
public:
	virtual ~Mapping() {}
	Mapping(
		const char*									element) :
		XMLElementMapping(element) {}

	IPersistent* create() const {
		return new T();
	}
};

// compare pointers to Mappings by element name
struct _mapping_ptr_less : public std::binary_function<XMLElementMapping*,XMLElementMapping*,bool> {
public:
	bool operator()(const XMLElementMapping* a, const XMLElementMapping* b) const {
		return a->element < b->element;
	}
};


__END_CORTEX_NAMESPACE
#endif /*__XMLElementMapping_H__*/
