/*
 * Copyright, 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef _UDF_ARRAY_H
#define _UDF_ARRAY_H


#include "SupportDefs.h"
#include "UdfDebug.h"

/*! \brief Slightly more typesafe static array type than built-in arrays,
	with array length information stored implicitly (i.e. consuming no
	physical space in the actual struct) via the \c arrayLength template
	parameter.
*/
template<typename DataType, uint32 arrayLength>
struct array {
public:
	void dump() const {
		for (uint32 i = 0; i < arrayLength; i++)
			data[i].print();
	}
	
	uint32 length() const { return arrayLength; }
	uint32 size() const { return arrayLength * sizeof(DataType); }
	
	// This doesn't appear to work. I don't know why.
	DataType operator[] (int index) const { return data[index]; }
	
	DataType data[arrayLength];
};


/*! \brief \c uint8 specialization of the \c array template struct. */
template<uint32 arrayLength>
struct array<uint8, arrayLength> {
	void dump() const
	{
		const uint8 bytesPerRow = 8;
		char classname[40];
		sprintf(classname, "array<uint8, %" B_PRIu32 ">", arrayLength);
		
		DUMP_INIT(classname);
		
		for (uint32 i = 0; i < arrayLength; i++) {
			if (i % bytesPerRow == 0)
				PRINT(("[%" B_PRIu32 ":%" B_PRIu32 "]: ",
					i, i + bytesPerRow - 1));
			SIMPLE_PRINT(("0x%.2x ", data[i]));
			if ((i + 1) % bytesPerRow == 0 || i + 1 == arrayLength)
				SIMPLE_PRINT(("\n"));
		}
	}

	uint32 length() const { return arrayLength; }
	uint32 size() const { return arrayLength; }
	uint8 data[arrayLength];
};


/*! \brief \c char specialization of the \c array template struct. */
template<uint32 arrayLength>
struct array<char, arrayLength> {
	void dump() const
	{
		const uint8 bytesPerRow = 8;
		char classname[40];
		sprintf(classname, "array<uint8, %" B_PRIu32 ">", arrayLength);

		DUMP_INIT(classname);

		for (uint32 i = 0; i < arrayLength; i++) {
			if (i % bytesPerRow == 0)
				PRINT(("[%" B_PRIu32 ":%" B_PRIu32 "]: ",
					i, i + bytesPerRow - 1));
			SIMPLE_PRINT(("0x%.2x ", data[i]));
			if ((i + 1) % bytesPerRow == 0 || i + 1 == arrayLength)
				SIMPLE_PRINT(("\n"));
		}
	}

	uint32 length() const { return arrayLength; }
	uint32 size() const { return arrayLength; }
	uint8 data[arrayLength];
};

#endif	// _UDF_ARRAY_H
