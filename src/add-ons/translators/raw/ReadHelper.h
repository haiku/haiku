/*
 * Copyright 2004-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef READ_HELPER_H
#define READ_HELPER_H


#include "TIFF.h"

#include <BufferIO.h>
#include <ByteOrder.h>


template<class T>
inline void
byte_swap(T &/*data*/)
{
	// Specialize for data types which actually swap
//	printf("DEAD MEAT!\n");
//	exit(1);
}


template<>
inline void
byte_swap(float &data)
{
	data = __swap_float(data);
}


template<>
inline void
byte_swap(int32 &data)
{
	data = __swap_int32(data);
}


template<>
inline void
byte_swap(uint32 &data)
{
	data = __swap_int32(data);
}


template<>
inline void
byte_swap(int16 &data)
{
	data = __swap_int16(data);
}


template<>
inline void
byte_swap(uint16 &data)
{
	data = __swap_int16(data);
}


class TReadHelper {
	public:
		TReadHelper(BPositionIO& stream)
			:
			fStream(&stream, 65536, false),
			fError(B_OK),
			fSwap(false)
		{
		}

		template<class T> inline void
		operator()(T &data)
		{
			fError = fStream.Read((void *)&data, sizeof(T));
			if (fError >= B_OK) {
				if (IsSwapping())
					byte_swap(data);
				return;
			}

			if (fError == 0)
				fError = B_ERROR;
			throw fError;
		}

		template<class T> inline void
		operator()(T data, size_t length)
		{
			fError = fStream.Read((void *)data, length);
			if (fError < (ssize_t)length)
				fError = B_ERROR;

			if (fError >= B_OK)
				return;

			throw fError;
		}

		template<class T> inline T
		Next()
		{
			T value;
			fError = fStream.Read((void *)&value, sizeof(T));
			if (fError > B_OK) {
				if (IsSwapping())
					byte_swap(value);
				return value;
			}

			if (fError == 0)
				fError = B_ERROR;
			throw fError;
		}

		inline uint32
		Next(uint16 type)
		{
			if (type == TIFF_UINT16_TYPE || type == TIFF_INT16_TYPE)
				return Next<uint16>();

			return Next<uint32>();
		}

		inline double
		NextDouble(uint16 type)
		{
			switch (type) {
				case TIFF_UINT16_TYPE:
					return Next<uint16>();
				case TIFF_UINT32_TYPE:
					return Next<uint32>();
				case TIFF_UFRACTION_TYPE:
				{
					double value = Next<uint32>();
					return value / Next<uint32>();
				}
				case TIFF_INT16_TYPE:
					return Next<int16>();
				case TIFF_INT32_TYPE:
					return Next<int32>();
				case TIFF_FRACTION_TYPE:
				{
					double value = Next<int32>();
					return value / Next<int32>();
				}
				case TIFF_FLOAT_TYPE:
					return Next<float>();
				case TIFF_DOUBLE_TYPE:
					return Next<double>();

				default:
					return Next<uint8>();
			}
		}

		inline void
		NextShorts(uint16* data, size_t length)
		{
			fError = fStream.Read(data, length * 2);
			if (fError < (ssize_t)length * 2)
				fError = B_ERROR;

			if (fError >= B_OK) {
				if (IsSwapping())
					swap_data(B_INT16_TYPE, data, length * 2, B_SWAP_ALWAYS);
				return;
			}

			throw fError;
		}

		status_t Status() const
			{ return fError >= B_OK ? B_OK : fError; };

		off_t Seek(off_t offset, int32 mode)
			{ return fStream.Seek(offset, mode); }
		off_t Position() const
			{ return fStream.Position(); }

		void SetSwap(bool yesNo) { fSwap = yesNo; };
		bool IsSwapping() const { return fSwap; };

		BPositionIO& Stream() { return fStream; }

	private:
		BBufferIO	fStream;
		status_t	fError;
		bool		fSwap;
};

#endif	// READ_HELPER_H
