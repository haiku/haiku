#include <algorithm>

#include "All.h"
#include "APEInfo.h"
#include "UnBitArray.h"
#include "BitArray.h"


const uint32 POWERS_OF_TWO_MINUS_ONE_REVERSED[33] = {4294967295u,2147483647u,
	1073741823u,536870911u,268435455u,134217727u,67108863u,33554431u,16777215u,
	8388607u,4194303u,2097151u,1048575u,524287u,262143u,131071u,65535u,32767u,
	16383u,8191u,4095u,2047u,1023u,511u,255u,127u,63u,31u,15u,7u,3u,1u,0u};

const uint32 K_SUM_MIN_BOUNDARY[32] = {0u,32u,64u,128u,256u,512u,1024u,2048u,
	4096u,8192u,16384u,32768u,65536u,131072u,262144u,524288u,1048576u,2097152u,
	4194304u,8388608u,16777216u,33554432u,67108864u,134217728u,268435456u,
	536870912u,1073741824u,2147483648u,0u,0u,0u,0u};

const uint32 RANGE_TOTAL_1[65] = {0u,14824u,28224u,39348u,47855u,53994u,58171u,
	60926u,62682u,63786u,64463u,64878u,65126u,65276u,65365u,65419u,65450u,
	65469u,65480u,65487u,65491u,65493u,65494u,65495u,65496u,65497u,65498u,
	65499u,65500u,65501u,65502u,65503u,65504u,65505u,65506u,65507u,65508u,
	65509u,65510u,65511u,65512u,65513u,65514u,65515u,65516u,65517u,65518u,
	65519u,65520u,65521u,65522u,65523u,65524u,65525u,65526u,65527u,65528u,
	65529u,65530u,65531u,65532u,65533u,65534u,65535u,65536u};
	
const uint32 RANGE_WIDTH_1[64] = {14824u,13400u,11124u,8507u,6139u,4177u,2755u,
	1756u,1104u,677u,415u,248u,150u,89u,54u,31u,19u,11u,7u,4u,2u,1u,1u,1u,1u,1u,
	1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
	1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u};

const uint32 RANGE_TOTAL_2[65] = {0u,19578u,36160u,48417u,56323u,60899u,63265u,
	64435u,64971u,65232u,65351u,65416u,65447u,65466u,65476u,65482u,65485u,
	65488u,65490u,65491u,65492u,65493u,65494u,65495u,65496u,65497u,65498u,
	65499u,65500u,65501u,65502u,65503u,65504u,65505u,65506u,65507u,65508u,
	65509u,65510u,65511u,65512u,65513u,65514u,65515u,65516u,65517u,65518u,
	65519u,65520u,65521u,65522u,65523u,65524u,65525u,65526u,65527u,65528u,
	65529u,65530u,65531u,65532u,65533u,65534u,65535u,65536u};

const uint32 RANGE_WIDTH_2[64] = {19578u,16582u,12257u,7906u,4576u,2366u,1170u,
	536u,261u,119u,65u,31u,19u,10u,6u,3u,3u,2u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
	1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
	1u,1u,1u,1u,1u,1u,1u,1u,1u,1u};


#define RANGE_OVERFLOW_TOTAL_WIDTH	65536
#define RANGE_OVERFLOW_SHIFT		16

#define CODE_BITS 		32
#define TOP_VALUE		((unsigned int ) 1 << (CODE_BITS - 1))
#define SHIFT_BITS		(CODE_BITS - 9)
#define EXTRA_BITS		((CODE_BITS - 2) % 8 + 1)
#define BOTTOM_VALUE	(TOP_VALUE >> 8)

#define MODEL_ELEMENTS	64

#if __GNUC__ != 2
using std::min;
using std::max;
#endif


CUnBitArray::CUnBitArray(CIO * pIO, int nVersion)
{
	CreateHelper(pIO, 16384, nVersion);
	m_nFlushCounter = 0;
	m_nFinalizeCounter = 0;
}


CUnBitArray::~CUnBitArray()
{
	SAFE_ARRAY_DELETE(m_pBitArray)
}


unsigned int
CUnBitArray::DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1,
	int nParam2)
{
	switch (DecodeMethod) {
		case DECODE_VALUE_METHOD_UNSIGNED_INT:
			return DecodeValueXBits(32);
		case DECODE_VALUE_METHOD_UNSIGNED_RICE:
		case DECODE_VALUE_METHOD_X_BITS:
			break;
	}

	return 0;
}


void
CUnBitArray::GenerateArray(int * pOutputArray, int nElements,
	int nBytesRequired)
{
	GenerateArrayRange(pOutputArray, nElements);
}

__inline unsigned char
CUnBitArray::GetC()
{
	uchar nValue = static_cast<uchar>(m_pBitArray[m_nCurrentBitIndex >> 5]
		>> (24 - (m_nCurrentBitIndex & 31)));
	m_nCurrentBitIndex += 8;
	return nValue;
}


__inline int
CUnBitArray::RangeDecodeFast(int nShift)
{
	while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
		m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8)
			| ((m_pBitArray[m_nCurrentBitIndex >> 5] 
			>> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
		m_nCurrentBitIndex += 8;
		m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8)
			| ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
		m_RangeCoderInfo.range <<= 8;
	}

	// decode
	m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;
	return m_RangeCoderInfo.low / m_RangeCoderInfo.range;
}


__inline int CUnBitArray::RangeDecodeFastWithUpdate(int nShift)
{
	while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
		m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8)
			| ((m_pBitArray[m_nCurrentBitIndex >> 5]
			>> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
		m_nCurrentBitIndex += 8;
		m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8)
			| ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
		m_RangeCoderInfo.range <<= 8;
	}

	// decode
	m_RangeCoderInfo.range = m_RangeCoderInfo.range >> nShift;
	int nRetVal = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
	m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nRetVal;
	return nRetVal;
}


int
CUnBitArray::DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState)
{
	// make sure there is room for the data
	// this is a little slower than ensuring a huge block to start with,
	// but it's safer
	if (m_nCurrentBitIndex > m_nRefillBitThreshold)
		FillBitArray();

	int nValue = 0;

	if (m_nVersion >= 3990) {
		// figure the pivot value
		int nPivotValue = max(BitArrayState.nKSum / 32, 1UL);

		// get the overflow
		int nOverflow = 0;

		// decode
		uint32 nRangeTotal = RangeDecodeFast(RANGE_OVERFLOW_SHIFT);

		// lookup the symbol (must be a faster way than this)
		while (nRangeTotal >= RANGE_TOTAL_2[nOverflow + 1])
			nOverflow++;

		// update
		m_RangeCoderInfo.low -= m_RangeCoderInfo.range 
			* RANGE_TOTAL_2[nOverflow];
		m_RangeCoderInfo.range = m_RangeCoderInfo.range 
			* RANGE_WIDTH_2[nOverflow];

		// get the working k
		if (nOverflow == (MODEL_ELEMENTS - 1)) {
			nOverflow = RangeDecodeFastWithUpdate(16);
			nOverflow <<= 16;
			nOverflow |= RangeDecodeFastWithUpdate(16);
		}

		// get the value
		int nBase = 0;

		if (nPivotValue >= (1 << 16)) {
			int nPivotValueBits = 0;
			while ((nPivotValue >> nPivotValueBits) > 0)
				nPivotValueBits++;
				
			int nSplitFactor = 1 << (nPivotValueBits - 16);
			int nPivotValueA = (nPivotValue / nSplitFactor) + 1;
			int nPivotValueB = nSplitFactor;

			while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8)
					| ((m_pBitArray[m_nCurrentBitIndex >> 5] 
					>> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
				m_nCurrentBitIndex += 8;
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8)
					| ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueA;
			int nBaseA = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseA;

			while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8)
					| ((m_pBitArray[m_nCurrentBitIndex >> 5] 
					>> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
				m_nCurrentBitIndex += 8;
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8)
					| ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValueB;
			int nBaseB = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseB;

			nBase = nBaseA * nSplitFactor + nBaseB;
		} else {
			while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
				m_RangeCoderInfo.buffer = (m_RangeCoderInfo.buffer << 8)
					| ((m_pBitArray[m_nCurrentBitIndex >> 5]
					>> (24 - (m_nCurrentBitIndex & 31))) & 0xFF);
				m_nCurrentBitIndex += 8;
				m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8)
					| ((m_RangeCoderInfo.buffer >> 1) & 0xFF);
				m_RangeCoderInfo.range <<= 8;
			}

			// decode
			m_RangeCoderInfo.range = m_RangeCoderInfo.range / nPivotValue;
			int nBaseLower = m_RangeCoderInfo.low / m_RangeCoderInfo.range;
			m_RangeCoderInfo.low -= m_RangeCoderInfo.range * nBaseLower;

			nBase = nBaseLower;
		}

		// build the value
		nValue = nBase + (nOverflow * nPivotValue);
	} else {
		// decode
		uint32 nRangeTotal = RangeDecodeFast(RANGE_OVERFLOW_SHIFT);

		// lookup the symbol (must be a faster way than this)
		int nOverflow = 0;
		while (nRangeTotal >= RANGE_TOTAL_1[nOverflow + 1])
			nOverflow++;

		// update
		m_RangeCoderInfo.low -= m_RangeCoderInfo.range
			* RANGE_TOTAL_1[nOverflow];
		m_RangeCoderInfo.range = m_RangeCoderInfo.range
			* RANGE_WIDTH_1[nOverflow];

		// get the working k
		int nTempK;
		if (nOverflow == (MODEL_ELEMENTS - 1)) {
			nTempK = RangeDecodeFastWithUpdate(5);
			nOverflow = 0;
		} else
			nTempK = (BitArrayState.k < 1) ? 0 : BitArrayState.k - 1;

		// figure the extra bits on the left and the left value
		if (nTempK <= 16 || m_nVersion < 3910) {
			nValue = RangeDecodeFastWithUpdate(nTempK);
		} else {
			int nX1 = RangeDecodeFastWithUpdate(16);
			int nX2 = RangeDecodeFastWithUpdate(nTempK - 16);
			nValue = nX1 | (nX2 << 16);
		}

		// build the value and output it
		nValue += (nOverflow << nTempK);
	}

	// update nKSum
	BitArrayState.nKSum += ((nValue + 1) / 2) 
		- ((BitArrayState.nKSum + 16) >> 5);

	// update k
	if (BitArrayState.nKSum < K_SUM_MIN_BOUNDARY[BitArrayState.k])
		BitArrayState.k--;
	else if (BitArrayState.nKSum >= K_SUM_MIN_BOUNDARY[BitArrayState.k + 1])
		BitArrayState.k++;

	// output the value (converted to signed)
	return (nValue & 1) ? (nValue >> 1) + 1 : -(nValue >> 1);
}


void
CUnBitArray::FlushState(UNBIT_ARRAY_STATE& BitArrayState)
{
	BitArrayState.k = 10;
	BitArrayState.nKSum = (1 << BitArrayState.k) * 16;
}

void
CUnBitArray::FlushBitArray()
{
	AdvanceToByteBoundary();
	m_nCurrentBitIndex += 8;
		// ignore the first byte... (slows compression too much to not
		// output this dummy byte)
	m_RangeCoderInfo.buffer = GetC();
	m_RangeCoderInfo.low = m_RangeCoderInfo.buffer >> (8 - EXTRA_BITS);
	m_RangeCoderInfo.range = (unsigned int) 1 << EXTRA_BITS;

	m_nRefillBitThreshold = (m_nBits - 512);
}


void
CUnBitArray::Finalize()
{
	// normalize
	while (m_RangeCoderInfo.range <= BOTTOM_VALUE) {
		m_nCurrentBitIndex += 8;
		m_RangeCoderInfo.range <<= 8;
	}

	// used to back-pedal the last two bytes out
	// this should never have been a problem because we've outputted and normalized beforehand
	// but stopped doing it as of 3.96 in case it accounted for rare decompression failures
	if (m_nVersion <= 3950)
		m_nCurrentBitIndex -= 16;
}


void
CUnBitArray::GenerateArrayRange(int * pOutputArray, int nElements)
{
	UNBIT_ARRAY_STATE BitArrayState;
	FlushState(BitArrayState);
	FlushBitArray();

	for (int z = 0; z < nElements; z++)
		pOutputArray[z] = DecodeValueRange(BitArrayState);

	Finalize();
}
