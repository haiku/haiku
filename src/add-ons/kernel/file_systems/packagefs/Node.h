/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <fs_interface.h>


class Node {
public:
								Node(ino_t id);
	virtual						~Node();

			ino_t				ID() const	{ return fID; }

private:
			ino_t				fID;
};


#endif	// NODE_H
