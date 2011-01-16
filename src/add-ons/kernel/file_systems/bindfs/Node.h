/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <fs_interface.h>


class Node {
public:
								Node(ino_t sourceID, mode_t mode);
								~Node();

			ino_t				ID() const			{ return fSourceID; }
									// currently, we reuse the source-ID

			ino_t				SourceID() const	{ return fSourceID; }
			mode_t				Mode() const		{ return fMode; }

protected:
			ino_t				fSourceID;
			mode_t				fMode;
};


#endif	// NODE_H
