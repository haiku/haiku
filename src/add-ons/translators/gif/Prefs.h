////////////////////////////////////////////////////////////////////////////////
//
//	File: Prefs.h
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

#ifndef PREFS_H
#define PREFS_H

class BNode;

class Prefs {
	public:
		Prefs();
		void Save();
		~Prefs();
		bool interlaced, usetransparent, usetransparentauto,
			usedithering;
		int transparentred, transparentgreen,
			transparentblue, palettemode, palette_size_in_bits;
	private:
		bool GetInt(const char *name, int *value, int *defaultvalue);
		bool GetBool(const char *name, bool *value, bool *defaultvalue);
		bool PutInt(const char *name, int *value);
		bool PutBool(const char *name, bool *value);
		
		BNode *file;
};

#endif

