#include "All.h"
#include "Prepare.h"


const uint32 CRC32_TABLE[256] = {0u,1996959894u,3993919788u,2567524794u,
	124634137u,1886057615u,3915621685u,2657392035u,249268274u,2044508324u,
	3772115230u,2547177864u,162941995u,2125561021u,3887607047u,2428444049u,
	498536548u,1789927666u,4089016648u,2227061214u,450548861u,1843258603u,
	4107580753u,2211677639u,325883990u,1684777152u,4251122042u,2321926636u,
	335633487u,1661365465u,4195302755u,2366115317u,997073096u,1281953886u,
	3579855332u,2724688242u,1006888145u,1258607687u,3524101629u,2768942443u,
	901097722u,1119000684u,3686517206u,2898065728u,853044451u,1172266101u,
	3705015759u,2882616665u,651767980u,1373503546u,3369554304u,3218104598u,
	565507253u,1454621731u,3485111705u,3099436303u,671266974u,1594198024u,
	3322730930u,2970347812u,795835527u,1483230225u,3244367275u,3060149565u,
	1994146192u,31158534u,2563907772u,4023717930u,1907459465u,112637215u,
	2680153253u,3904427059u,2013776290u,251722036u,2517215374u,3775830040u,
	2137656763u,141376813u,2439277719u,3865271297u,1802195444u,476864866u,
	2238001368u,4066508878u,1812370925u,453092731u,2181625025u,4111451223u,
	1706088902u,314042704u,2344532202u,4240017532u,1658658271u,366619977u,
	2362670323u,4224994405u,1303535960u,984961486u,2747007092u,3569037538u,
	1256170817u,1037604311u,2765210733u,3554079995u,1131014506u,879679996u,
	2909243462u,3663771856u,1141124467u,855842277u,2852801631u,3708648649u,
	1342533948u,654459306u,3188396048u,3373015174u,1466479909u,544179635u,
	3110523913u,3462522015u,1591671054u,702138776u,2966460450u,3352799412u,
	1504918807u,783551873u,3082640443u,3233442989u,3988292384u,2596254646u,
	62317068u,1957810842u,3939845945u,2647816111u,81470997u,1943803523u,
	3814918930u,2489596804u,225274430u,2053790376u,3826175755u,2466906013u,
	167816743u,2097651377u,4027552580u,2265490386u,503444072u,1762050814u,
	4150417245u,2154129355u,426522225u,1852507879u,4275313526u,2312317920u,
	282753626u,1742555852u,4189708143u,2394877945u,397917763u,1622183637u,
	3604390888u,2714866558u,953729732u,1340076626u,3518719985u,2797360999u,
	1068828381u,1219638859u,3624741850u,2936675148u,906185462u,1090812512u,
	3747672003u,2825379669u,829329135u,1181335161u,3412177804u,3160834842u,
	628085408u,1382605366u,3423369109u,3138078467u,570562233u,1426400815u,
	3317316542u,2998733608u,733239954u,1555261956u,3268935591u,3050360625u,
	752459403u,1541320221u,2607071920u,3965973030u,1969922972u,40735498u,
	2617837225u,3943577151u,1913087877u,83908371u,2512341634u,3803740692u,
	2075208622u,213261112u,2463272603u,3855990285u,2094854071u,198958881u,
	2262029012u,4057260610u,1759359992u,534414190u,2176718541u,4139329115u,
	1873836001u,414664567u,2282248934u,4279200368u,1711684554u,285281116u,
	2405801727u,4167216745u,1634467795u,376229701u,2685067896u,3608007406u,
	1308918612u,956543938u,2808555105u,3495958263u,1231636301u,1047427035u,
	2932959818u,3654703836u,1088359270u,936918000u,2847714899u,3736837829u,
	1202900863u,817233897u,3183342108u,3401237130u,1404277552u,615818150u,
	3134207493u,3453421203u,1423857449u,601450431u,3009837614u,3294710456u,
	1567103746u,711928724u,3020668471u,3272380065u,1510334235u,755167117u};


int
CPrepare::Prepare(const unsigned char* pRawData, int nBytes,
	const WAVEFORMATEX* pWaveFormatEx, int* pOutputX, int* pOutputY,
	unsigned int* pCRC, int* pSpecialCodes, int* pPeakLevel)
{
	// error check the parameters
	if (pRawData == NULL || pWaveFormatEx == NULL)
		return ERROR_BAD_PARAMETER;

	// initialize the pointers that got passed in
	*pCRC = 0xFFFFFFFF;
	*pSpecialCodes = 0;

	// variables
	uint32 CRC = 0xFFFFFFFF;
	const int nTotalBlocks = nBytes / pWaveFormatEx->nBlockAlign;
	int R,L;

	// the prepare code
	if (pWaveFormatEx->wBitsPerSample == 8) {
		if (pWaveFormatEx->nChannels == 2) {
			for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks;
				nBlockIndex++) {

				R = (int) (*((unsigned char *) pRawData) - 128);
				L = (int) (*((unsigned char *) (pRawData + 1)) - 128);

				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
                
				// check the peak
				if (labs(L) > *pPeakLevel)
					*pPeakLevel = labs(L);
				if (labs(R) > *pPeakLevel)
					*pPeakLevel = labs(R);

				// convert to x,y
				pOutputY[nBlockIndex] = L - R;
				pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);
			}
		} else if (pWaveFormatEx->nChannels == 1) {
			for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks;
				nBlockIndex++) {

				R = (int) (*((unsigned char *) pRawData) - 128);

				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				// check the peak
				if (labs(R) > *pPeakLevel)
					*pPeakLevel = labs(R);

				// convert to x,y
				pOutputX[nBlockIndex] = R;
			}
		}
	} else if (pWaveFormatEx->wBitsPerSample == 24) {
		if (pWaveFormatEx->nChannels == 2) {
			for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) {
				uint32 nTemp = 0;

				nTemp |= (*pRawData << 0);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 8);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 16);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				if (nTemp & 0x800000)
					R = (int) (nTemp & 0x7FFFFF) - 0x800000;
				else
					R = (int) (nTemp & 0x7FFFFF);

				nTemp = 0;

				nTemp |= (*pRawData << 0);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 8);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 16);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				if (nTemp & 0x800000)
					L = (int) (nTemp & 0x7FFFFF) - 0x800000;
				else
					L = (int) (nTemp & 0x7FFFFF);

				// check the peak
				if (labs(L) > *pPeakLevel)
					*pPeakLevel = labs(L);
				if (labs(R) > *pPeakLevel)
					*pPeakLevel = labs(R);

				// convert to x,y
				pOutputY[nBlockIndex] = L - R;
				pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);
			}
		} else if (pWaveFormatEx->nChannels == 1) {
			for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks;
				nBlockIndex++) {

				uint32 nTemp = 0;

				nTemp |= (*pRawData << 0);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 8);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				nTemp |= (*pRawData << 16);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				if (nTemp & 0x800000)
					R = (int) (nTemp & 0x7FFFFF) - 0x800000;
				else
					R = (int) (nTemp & 0x7FFFFF);

				// check the peak
				if (labs(R) > *pPeakLevel)
					*pPeakLevel = labs(R);

				// convert to x,y
				pOutputX[nBlockIndex] = R;
			}
		}
	} else {
		if (pWaveFormatEx->nChannels == 2) {
			int LPeak = 0;
			int RPeak = 0;
			int nBlockIndex = 0;
			for (nBlockIndex = 0; nBlockIndex < nTotalBlocks; nBlockIndex++) {
				R = (int) *((int16 *) pRawData);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				L = (int) *((int16 *) pRawData);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				// check the peak
				if (labs(L) > LPeak)
					LPeak = labs(L);
				if (labs(R) > RPeak)
					RPeak = labs(R);

				// convert to x,y
				pOutputY[nBlockIndex] = L - R;
				pOutputX[nBlockIndex] = R + (pOutputY[nBlockIndex] / 2);
			}

			if (LPeak == 0)
				*pSpecialCodes |= SPECIAL_FRAME_LEFT_SILENCE;
			if (RPeak == 0)
				*pSpecialCodes |= SPECIAL_FRAME_RIGHT_SILENCE;
			if (max(LPeak, RPeak) > *pPeakLevel)
				*pPeakLevel = max(LPeak, RPeak);

			// check for pseudo-stereo files
			nBlockIndex = 0;
			while (pOutputY[nBlockIndex++] == 0) {
				if (nBlockIndex == (nBytes / 4)) {
					*pSpecialCodes |= SPECIAL_FRAME_PSEUDO_STEREO;
					break;
				}
			}
		} else if (pWaveFormatEx->nChannels == 1) {
			int nPeak = 0;
			for (int nBlockIndex = 0; nBlockIndex < nTotalBlocks;
				nBlockIndex++) {

				R = (int) *((int16 *) pRawData);
                
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *pRawData++];

				// check the peak
				if (labs(R) > nPeak)
					nPeak = labs(R);

				//convert to x,y
				pOutputX[nBlockIndex] = R;
			}

			if (nPeak > *pPeakLevel)
				*pPeakLevel = nPeak;

			if (nPeak == 0)
				*pSpecialCodes |= SPECIAL_FRAME_MONO_SILENCE;
		}
	}

	CRC = CRC ^ 0xFFFFFFFF;

	// add the special code
	CRC >>= 1;

	if (*pSpecialCodes != 0) 
		CRC |= (1 << 31);

	*pCRC = CRC;

	return ERROR_SUCCESS;
}


void
CPrepare::Unprepare(int X, int Y, const WAVEFORMATEX* pWaveFormatEx,
	unsigned char* pOutput, unsigned int* pCRC)
{
	#define CALCULATE_CRC_BYTE    *pCRC = (*pCRC >> 8) ^ CRC32_TABLE[(*pCRC & 0xFF) ^ *pOutput++];

	// decompress and convert from (x,y) -> (l,r)
	// sort of long and ugly.... sorry

	if (pWaveFormatEx->nChannels == 2) {
		if (pWaveFormatEx->wBitsPerSample == 16) {
			// get the right and left values
			int nR = X - (Y / 2);
			int nL = nR + Y;

			// error check (for overflows)
			if ((nR < -32768) || (nR > 32767) || (nL < -32768) || (nL > 32767))
				throw(-1);

			*(int16 *) pOutput = (int16) nR;
			CALCULATE_CRC_BYTE
			CALCULATE_CRC_BYTE
                
			*(int16 *) pOutput = (int16) nL;
			CALCULATE_CRC_BYTE
			CALCULATE_CRC_BYTE
		} else if (pWaveFormatEx->wBitsPerSample == 8) {
			unsigned char R = (X - (Y / 2) + 128);
			*pOutput = R;
			CALCULATE_CRC_BYTE
			*pOutput = (unsigned char) (R + Y);
			CALCULATE_CRC_BYTE
		} else if (pWaveFormatEx->wBitsPerSample == 24) {
			int32 RV, LV;

			RV = X - (Y / 2);
			LV = RV + Y;

			uint32 nTemp = 0;
			if (RV < 0)
				nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
			else
				nTemp = (uint32) RV;    

			*pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
			CALCULATE_CRC_BYTE
			*pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
			CALCULATE_CRC_BYTE
			*pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
			CALCULATE_CRC_BYTE

			nTemp = 0;
			if (LV < 0)
				nTemp = ((uint32) (LV + 0x800000)) | 0x800000;
			else
				nTemp = (uint32) LV;    

			*pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
			CALCULATE_CRC_BYTE

			*pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
			CALCULATE_CRC_BYTE

			*pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
			CALCULATE_CRC_BYTE
		}
	} else if (pWaveFormatEx->nChannels == 1) {
		if (pWaveFormatEx->wBitsPerSample == 16) {
			int16 R = X;
    
			*(int16 *) pOutput = (int16) R;
			CALCULATE_CRC_BYTE
			CALCULATE_CRC_BYTE
		} else if (pWaveFormatEx->wBitsPerSample == 8) {
			unsigned char R = X + 128;
			*pOutput = R;
			CALCULATE_CRC_BYTE
		} else if (pWaveFormatEx->wBitsPerSample == 24) {
			int32 RV = X;
            uint32 nTemp = 0;

			if (RV < 0)
				nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
			else
				nTemp = (uint32) RV;    

			*pOutput = (unsigned char) ((nTemp >> 0) & 0xFF);
			CALCULATE_CRC_BYTE
			*pOutput = (unsigned char) ((nTemp >> 8) & 0xFF);
			CALCULATE_CRC_BYTE
			*pOutput = (unsigned char) ((nTemp >> 16) & 0xFF);
			CALCULATE_CRC_BYTE
		}
	}
}


#ifdef BACKWARDS_COMPATIBILITY
int
CPrepare::UnprepareOld(int* pInputX, int* pInputY, int nBlocks,
	const WAVEFORMATEX* pWaveFormatEx, unsigned char* pRawData,
	unsigned int* pCRC, int* pSpecialCodes, int nFileVersion)
{
	// the CRC that will be figured during decompression
	uint32 CRC = 0xFFFFFFFF;

	// decompress and convert from (x,y) -> (l,r)
	// sort of int and ugly.... sorry
	if (pWaveFormatEx->nChannels == 2) {
		// convert the x,y data to raw data
		if (pWaveFormatEx->wBitsPerSample == 16) {
			int16 R;
			unsigned char *Buffer = &pRawData[0];
			int *pX = pInputX;
			int *pY = pInputY;

			for (; pX < &pInputX[nBlocks]; pX++, pY++) {
				R = *pX - (*pY / 2);

				*(int16 *) Buffer = (int16) R;
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*(int16 *) Buffer = (int16) R + *pY;
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
			}
		} else if (pWaveFormatEx->wBitsPerSample == 8) {
			unsigned char *R = (unsigned char *) &pRawData[0];
			unsigned char *L = (unsigned char *) &pRawData[1];

			if (nFileVersion > 3830) {
				for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++,
					L+=2, R+=2) {

					*R = (unsigned char) (pInputX[SampleIndex] 
						- (pInputY[SampleIndex] / 2) + 128);
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
					*L = (unsigned char) (*R + pInputY[SampleIndex]);
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *L];
				}
			} else {
				for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++,
					L+=2, R+=2) {

					*R = (unsigned char) (pInputX[SampleIndex]
						- (pInputY[SampleIndex] / 2));
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
					*L = (unsigned char) (*R + pInputY[SampleIndex]);
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *L];
				}
			}
		} else if (pWaveFormatEx->wBitsPerSample == 24) {
			unsigned char *Buffer = (unsigned char *) &pRawData[0];
			int32 RV, LV;

			for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++) {
				RV = pInputX[SampleIndex] - (pInputY[SampleIndex] / 2);
				LV = RV + pInputY[SampleIndex];

				uint32 nTemp = 0;
				if (RV < 0)
					nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
				else
					nTemp = (uint32) RV;    

				*Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				nTemp = 0;
				if (LV < 0)
					nTemp = ((uint32) (LV + 0x800000)) | 0x800000;
				else
					nTemp = (uint32) LV;    

				*Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
			}
		}
	} else if (pWaveFormatEx->nChannels == 1) {
		// convert to raw data
		if (pWaveFormatEx->wBitsPerSample == 8) {
			unsigned char *R = (unsigned char *) &pRawData[0];

			if (nFileVersion > 3830) {
				for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++,
					R++) {

					*R = pInputX[SampleIndex] + 128;
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
				}
			} else {
				for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++,
					R++) {

					*R = (unsigned char) (pInputX[SampleIndex]);
					CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *R];
				}
			}
		} else if (pWaveFormatEx->wBitsPerSample == 24) {
			unsigned char *Buffer = (unsigned char *) &pRawData[0];
			int32 RV;
			for (int SampleIndex = 0; SampleIndex<nBlocks; SampleIndex++) {
				RV = pInputX[SampleIndex];

				uint32 nTemp = 0;
				if (RV < 0)
					nTemp = ((uint32) (RV + 0x800000)) | 0x800000;
				else
					nTemp = (uint32) RV;    

				*Buffer = (unsigned char) ((nTemp >> 0) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 8) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];

				*Buffer = (unsigned char) ((nTemp >> 16) & 0xFF);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
			}
		} else {
			unsigned char *Buffer = &pRawData[0];

			for (int SampleIndex = 0; SampleIndex < nBlocks; SampleIndex++) {
				*(int16 *) Buffer = (int16) (pInputX[SampleIndex]);
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
				CRC = (CRC >> 8) ^ CRC32_TABLE[(CRC & 0xFF) ^ *Buffer++];
			}
		}
	}

	CRC = CRC ^ 0xFFFFFFFF;

	*pCRC = CRC;

	return 0;
}
#endif // #ifdef BACKWARDS_COMPATIBILITY
