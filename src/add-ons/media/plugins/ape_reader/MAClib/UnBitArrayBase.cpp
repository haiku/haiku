#include "All.h"
#include "UnBitArrayBase.h"
#include "APEInfo.h"
#include "UnBitArray.h"


#undef	BACKWARDS_COMPATIBILITY

#ifdef BACKWARDS_COMPATIBILITY
	#include "Old/APEDecompressOld.h"
	#include "Old/UnBitArrayOld.h"
#endif


const uint32 POWERS_OF_TWO_MINUS_ONE[33] = {0u,1u,3u,7u,15u,31u,63u,127u,255u,
	511u,1023u,2047u,4095u,8191u,16383u,32767u,65535u,131071u,262143u,524287u,
	1048575u,2097151u,4194303u,8388607u,16777215u,33554431u,67108863u,
	134217727u,268435455u,536870911u,1073741823u,2147483647u,4294967295u};


CUnBitArrayBase*
CreateUnBitArray(IAPEDecompress* pAPEDecompress, int nVersion)
{
#ifdef BACKWARDS_COMPATIBILITY
	if (nVersion >= 3900)
		return static_cast<CUnBitArrayBase*>
			(new CUnBitArray(GET_IO(pAPEDecompress), nVersion));
	else
		return static_cast<CUnBitArrayBase*>
			(new CUnBitArrayOld(pAPEDecompress, nVersion));
#else
	return static_cast<CUnBitArrayBase*>
		(new CUnBitArray(GET_IO(pAPEDecompress), nVersion));
#endif
}


void
CUnBitArrayBase::AdvanceToByteBoundary() 
{
	int nMod = m_nCurrentBitIndex % 8;
	if (nMod != 0)
		m_nCurrentBitIndex += 8 - nMod;
}


uint32
CUnBitArrayBase::DecodeValueXBits(uint32 nBits) 
{
	// get more data if necessary
	if ((m_nCurrentBitIndex + nBits) >= m_nBits)
		FillBitArray();

	// variable declares
	uint32 nLeftBits = 32 - (m_nCurrentBitIndex & 31);
	uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
	m_nCurrentBitIndex += nBits;

	// if there isn't an overflow to the right value, get the value and exit
	if (nLeftBits >= nBits)
		return (m_pBitArray[nBitArrayIndex] 
			& (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);

	// must get the "split" value from left and right
	int nRightBits = nBits - nLeftBits;
	uint32 nLeftValue = ((m_pBitArray[nBitArrayIndex]
		& POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
	uint32 nRightValue = (m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
	return (nLeftValue | nRightValue);
}


int
CUnBitArrayBase::FillAndResetBitArray(int nFileLocation, int nNewBitIndex) 
{
	// reset the bit index
	m_nCurrentBitIndex = nNewBitIndex;

	// seek if necessary
	if (nFileLocation != -1 && m_pIO->Seek(nFileLocation, FILE_BEGIN) != 0)
		return ERROR_IO_READ;

    // read the new data into the bit array
	unsigned int nBytesRead = 0;
	if (m_pIO->Read((unsigned char*)m_pBitArray, m_nBytes, 
		&nBytesRead) != 0)
		return ERROR_IO_READ;
    return 0;
}


int
CUnBitArrayBase::FillBitArray() 
{
	// get the bit array index
	uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;

	// move the remaining data to the front
	memmove((void *) (m_pBitArray),	static_cast<const void*>(m_pBitArray
		+ nBitArrayIndex), m_nBytes - (nBitArrayIndex * 4));

	// read the new data
	int nBytesToRead = nBitArrayIndex * 4;
	unsigned int nBytesRead = 0;
	int nRetVal = m_pIO->Read((unsigned char*)(m_pBitArray + m_nElements
		- nBitArrayIndex), nBytesToRead, &nBytesRead);

	// adjust the m_Bit pointer
	m_nCurrentBitIndex = m_nCurrentBitIndex & 31;

	// return
	return (nRetVal == 0) ? 0 : ERROR_IO_READ;
}


int
CUnBitArrayBase::CreateHelper(CIO * pIO, int nBytes, int nVersion)
{
	// check the parameters
	if ((pIO == NULL) || (nBytes <= 0))
		return ERROR_BAD_PARAMETER;

	// save the size
	m_nElements = nBytes / 4;
	m_nBytes = m_nElements * 4;
	m_nBits = m_nBytes * 8;

	// set the variables
	m_pIO = pIO;
	m_nVersion = nVersion;
	m_nCurrentBitIndex = 0;

	// create the bitarray
	m_pBitArray = new uint32[m_nElements];

	return (m_pBitArray != NULL) ? 0 : ERROR_INSUFFICIENT_MEMORY;
}
