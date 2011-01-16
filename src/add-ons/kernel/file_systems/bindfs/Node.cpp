/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "Node.h"


Node::Node(ino_t id, mode_t mode)
	:
	fSourceID(id),
	fMode(mode)
{
}


Node::~Node()
{
}
