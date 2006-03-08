// debug_tools.cpp

#include "debug_tools.h"

#include <stdlib.h>
#include <Alert.h>
#include <stdarg.h>

#include <strstream>

string point_to_string(const BPoint& p) {
	strstream out;
	out << '(' << p.x << ',' << p.y << ')';
	return out.str();
}

string rect_to_string(const BRect& r) {
	strstream out;
	out << '(' << r.left << ',' << r.top <<
		")-(" << r.right << ',' << r.bottom << ')';
	return out.str();
}

string id_to_string(uint32 nId) {
	string out;
	out += (char)(nId >> 24);
	out += (char)(nId >> 16) & 0xff;
	out += (char)(nId >> 8) & 0xff;
	out += (char)nId & 0xff;
	return out;
}


void show_alert(char* pFormat, ...) {
	// +++++
}


// END -- debug_tools.h --
