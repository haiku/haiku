/*
 * Compress3.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


// #define DBG_CON_STREAM

#ifdef DBG_CON_STREAM
#include <fstream>
using namespace std;
#endif

#include "Compress3.h"

#define PACK_TYPE	2


int
compress3(unsigned char* pOut, unsigned char* pIn, int n)
{
	int count;
	int count_byte;
	unsigned char* pBase;
	unsigned char* pLast;
	unsigned char keep;

	count = 0;
	count_byte = 0;

	keep   = ~(*pIn);
	pLast  = pIn + n;

	for (pBase = pIn; (pBase < pLast) && (count_byte < n) ; pBase++) {
		if (*pBase == keep) {
			if (count < 0xff + PACK_TYPE) {
				count++;
			} else {
				*pOut++ = keep;
				*pOut++ = count - PACK_TYPE;
				count = 1;
				keep  = *pOut++ = *pBase;
				count_byte += 3;
			}
		} else {
			if (count > 1) {
				*pOut++ = keep;
				*pOut++ = count - PACK_TYPE;
				count_byte += 2;
			}
			count = 1;
			keep  = *pOut++ = *pBase;
			count_byte++;
		}
	}

	if (count > 1) {
		if ((count_byte + 2) < n) {
			*pOut++ = keep;
			*pOut++ = count - PACK_TYPE;
			count_byte += 2;
		} else
			count_byte = n;
	}

	return count_byte;
}


#ifdef DBG_CON_STREAM

int
main(int argc, char **argv)
{
	if (argc < 2)
		return -1;

	ifstream ifs(*++argv, ios::binary | ios::nocreate);
	if (!ifs)
		return -1;

	ifs.seekg(0, ios::end);
	long size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	unsigned char* pIn  = new unsigned char[size];
	unsigned char* pOut = new unsigned char[size * 3];

	ifs.read(pIn, size);

	int cnt = PackBits(pOut, pIn, size);

	ofstream ofs("test.bin", ios::binary);
	ofs.write(pOut, cnt);

	delete [] pIn;
	delete [] pOut;

}

#endif
