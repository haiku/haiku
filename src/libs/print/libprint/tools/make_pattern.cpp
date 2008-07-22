/*
 * make_pattern.cpp
 * Copyright 2000 Y.Takagi All Rights Reserved.
 */

#include <iostream>
#include <iomanip>

using namespace std;

#define MAX_HORTZ	4
#define MAX_VERT	4
#define MAX_ELEMENT	(MAX_HORTZ * MAX_VERT)

#include "original_dither_pattern.h"

void create_index_table(const unsigned char *p1, unsigned char *p2)
{
	for (int i = 0 ; i < MAX_ELEMENT ; i++) {
		p2[*p1] = i;
		p1++;
	}
}

inline int index2horz(int index)
{
	return index % MAX_HORTZ;
}

inline int index2vert(int index)
{
	return index / MAX_HORTZ;
}

void create_pattern16x16(const unsigned char *pattern4x4, unsigned char *pattern16x16)
{
	unsigned char value2index[MAX_ELEMENT];
	create_index_table(pattern4x4, value2index);

	for (int i = 0 ; i < MAX_ELEMENT ; i++) {
		int index = value2index[i];
		int h = index2horz(index);
		int v = index2vert(index);
		for (int j = 0 ; j < MAX_ELEMENT ; j++) {
			int index2 = value2index[j];
			int h2 = index2horz(index2) * 4 + h;
			int v2 = index2vert(index2) * 4 + v;
			pattern16x16[h2 + v2 * MAX_ELEMENT] = j + i * MAX_ELEMENT;
		}
	}
}

void print_pattern(ostream &os, const char *name, const unsigned char *pattern)
{
	os << "const unsigned char " << name << "[] = {" << '\n' << '\t';
	for (int i = 0 ; i < 256 ; i++) {
		os << setw(3) << (int)pattern[i];
		if (i == MAX_ELEMENT * MAX_ELEMENT - 1) {
			os << '\n';
		} else {
			os << ',';
			if (i % MAX_ELEMENT == MAX_ELEMENT - 1) {
				os << '\n' << '\t';
			}
		}
	}
	os << "};" << '\n';
}

int main()
{
	unsigned char pattern16x16_type1[MAX_ELEMENT * MAX_ELEMENT];
	create_pattern16x16(pattern4x4_type1, pattern16x16_type1);
	print_pattern(cout, "pattern16x16_type1", pattern16x16_type1);

	cout << endl;

	unsigned char pattern16x16_type2[MAX_ELEMENT * MAX_ELEMENT];
	create_pattern16x16(pattern4x4_type2, pattern16x16_type2);
	print_pattern(cout, "pattern16x16_type2", pattern16x16_type2);

	cout << endl;

	unsigned char pattern16x16_type3[MAX_ELEMENT * MAX_ELEMENT];
	create_pattern16x16(pattern4x4_type3, pattern16x16_type3);
	print_pattern(cout, "pattern16x16_type3", pattern16x16_type3);

	cout << endl;
	return 0;
}
