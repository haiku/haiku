/*
 * PackBits.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

//#define DBG_CON_STREAM

#ifdef DBG_CON_STREAM
#include <fstream>
#endif

#include <string.h>
#include "PackBits.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

#define	MAXINBYTES		127
#define CONTROL1(i)		-i
#define CONTROL2(i)		i

enum STATUS {
	INITIAL,
	UNDECIDED,
	UNMATCHED,
	MATCHED
};

int pack_bits(unsigned char *pOut, unsigned char *pIn, int n)
{
	int i;
	unsigned char *control;
	unsigned char *runbuf;
	unsigned char thisbyte;
	unsigned char runbyte;
	STATUS status;

	i = 0;
	status  = INITIAL;
	control = runbuf = pOut;
	runbyte = *pIn++;

	while (--n) {
		thisbyte = *pIn++;
		switch (status) {
		case INITIAL:
			control   = runbuf++;
			*runbuf++ = runbyte;
			if (thisbyte == runbyte) {
				status = UNDECIDED;
			} else {
				runbyte = thisbyte;
				status  = UNMATCHED;
			}
			i = 1;
			break;

		case UNDECIDED:
			if (i == MAXINBYTES) {
				*control  = CONTROL2(i);
				*runbuf++ = runbyte;
				runbyte   = thisbyte;
				status    = INITIAL;
			} else if (thisbyte == runbyte) {
				if (i > 1) {
					*control = CONTROL2(i - 2);
					control  = runbuf - 1;
				}
				i = 2;
				status = MATCHED;
			} else {
				*runbuf++ = runbyte;
				runbyte   = thisbyte;
				status    = UNMATCHED;
				i++;
			}
			break;

		case UNMATCHED:
			if (i == MAXINBYTES) {
				*control = CONTROL2(i);
				status   = INITIAL;
			} else {
				if (thisbyte == runbyte) {
					status = UNDECIDED;
				}
				i++;
			}
			*runbuf++ = runbyte;
			runbyte   = thisbyte;
			break;

		case MATCHED:
			if ((thisbyte != runbyte) || (i == MAXINBYTES)) {
				runbuf    = control;
				*runbuf++ = CONTROL1(i);
				*runbuf++ = runbyte;
				runbyte   = thisbyte;
				status    = INITIAL;
			} else {
				i++;
			}
			break;
		}
	}

	switch (status) {
	case INITIAL:
		*runbuf++ = CONTROL2(1);
		break;
	case UNDECIDED:
	case UNMATCHED:
		*control  = CONTROL2(i);
		break;
	case MATCHED:
		runbuf    = control;
		*runbuf++ = CONTROL1(i);
		break;
	}
	*runbuf++ = runbyte;

	return runbuf - pOut;
}

#ifdef DBG_CON_STREAM
int main(int argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}

	ifstream ifs(*++argv, ios::binary | ios::nocreate);
	if (!ifs) {
		return -1;
	}

	ifs.seekg(0, ios::end);
	long size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	unsigned char *pIn  = new unsigned char[size];
	unsigned char *pOut = new unsigned char[size * 3];

	ifs.read(pIn, size);

	int cnt = PackBits(pOut, pIn, size);

	ofstream ofs("test.bin", ios::binary);
	ofs.write(pOut, cnt);

	delete [] pIn;
	delete [] pOut;

}
#endif
