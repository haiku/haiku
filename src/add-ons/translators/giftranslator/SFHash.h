////////////////////////////////////////////////////////////////////////////////
//
//	File: SFHash.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>

#ifndef SFHASH_H
#define SFHASH_H

class HashItem {
	friend class SFHash;
	public:
		unsigned int key;
	private:
		HashItem *next;
};

class SFHash {
    public:
						SFHash(int size = 4096);
						~SFHash();

		void			AddItem(HashItem *item);
		HashItem*		GetItem(unsigned int key);
		unsigned int	CountItems();
		HashItem*		NextItem();
		void			Rewind();

        bool fatalerror;

    private:
        int size, iterate_pos, iterate_depth;
        HashItem **main_array;
};

#endif

