/*
 * PackBits.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

//#define DBG_CON_STREAM

#ifdef DBG_CON_STREAM
#include <fstream>
#endif

#include <stdio.h>
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

#define WRITE_TO_RUN_BUF(byte) { if (write) *runbuf ++ = byte; else runbuf ++; }
#define WRITE_CONTROL(byte) { if (write) *control = byte; }

template <bool write>
class PackBits {
public:
	static int doIt(unsigned char *pOut, const unsigned char *pIn, int n)
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
				WRITE_TO_RUN_BUF(runbyte);
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
					WRITE_CONTROL(CONTROL2(i));
					WRITE_TO_RUN_BUF(runbyte);
					runbyte   = thisbyte;
					status    = INITIAL;
				} else if (thisbyte == runbyte) {
					if (i > 1) {
						WRITE_CONTROL(CONTROL2(i - 2));
						control  = runbuf - 1;
					}
					i = 2;
					status = MATCHED;
				} else {
					WRITE_TO_RUN_BUF(runbyte);
					runbyte   = thisbyte;
					status    = UNMATCHED;
					i++;
				}
				break;
	
			case UNMATCHED:
				if (i == MAXINBYTES) {
					WRITE_CONTROL(CONTROL2(i));
					status   = INITIAL;
				} else {
					if (thisbyte == runbyte) {
						status = UNDECIDED;
					}
					i++;
				}
				WRITE_TO_RUN_BUF(runbyte);
				runbyte   = thisbyte;
				break;
	
			case MATCHED:
				if ((thisbyte != runbyte) || (i == MAXINBYTES)) {
					runbuf    = control;
					WRITE_TO_RUN_BUF(CONTROL1(i));
					WRITE_TO_RUN_BUF(runbyte);
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
			WRITE_TO_RUN_BUF(CONTROL2(1));
			break;
		case UNDECIDED:
		case UNMATCHED:
			WRITE_CONTROL(CONTROL2(i));
			break;
		case MATCHED:
			runbuf    = control;
			WRITE_TO_RUN_BUF(CONTROL1(i));
			break;
		}
		WRITE_TO_RUN_BUF(runbyte);
	
		return runbuf - pOut;
	}
};


int pack_bits_size(const unsigned char *pIn, int n)
{
	PackBits<false> calculateSize;
	return calculateSize.doIt(NULL, pIn, n);
}

int pack_bits(unsigned char *pOut, const unsigned char *pIn, int n)
{
	PackBits<true> compress;
	return compress.doIt(pOut, pIn, n);
}

#ifdef DBG_CON_STREAM
int main(int argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}

	FILE *input = fopen(*++argv, "rb");
	if (input == NULL) {
		return -1;
	}

	FILE *output = fopen("rle.out", "wb");
	if (output == NULL) {
		fclose(input);
		return -1;
	}

	fseek(input, 0, SEEK_END);
	long size = ftell(input);
	fseek(input, 0, SEEK_SET);

	unsigned char *pIn  = new unsigned char[size];
	fread(pIn, size, 1, input);

	long outSize = pack_bits_size(pIn, size);
	printf("input size: %d\noutput size: %d\n", (int)size, (int)outSize);

	unsigned char *pOut = new unsigned char[outSize];

	int cnt = pack_bits(pOut, pIn, size);

	fwrite(pOut, cnt, 1, output);

	fclose(input);
	fclose(output);

	delete [] pIn;
	delete [] pOut;

}
#endif
