//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Attribute.h

	BDataIO wrapper around a given attribute for a file. (declarations)
*/

#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

#include <DataIO.h>
#include <Node.h>
#include <string>

class Attribute : public BDataIO {
public:
	Attribute(BNode &node, const char *attribute);
	virtual	ssize_t Read(void *buffer, size_t size);
	virtual	ssize_t Write(const void *buffer, size_t size);
private:
	BNode fNode;
	string fAttribute;
};

#endif	// _ATTRIBUTE_H
