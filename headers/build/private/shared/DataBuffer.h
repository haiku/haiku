//------------------------------------------------------------------------------
//	DataBuffer.h
//
//------------------------------------------------------------------------------

#ifndef DATABUFFER_H
#define DATABUFFER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

class BDataBuffer
{
	public:
		BDataBuffer(size_t len);
		BDataBuffer(const void* data, size_t len, bool copy = false);
		BDataBuffer(const BDataBuffer& rhs, bool copy = false);
		~BDataBuffer();

		BDataBuffer& operator=(const BDataBuffer& rhs);

		size_t		BufferSize() const;
		const void*	Buffer() const;

	private:
		class BDataReference
		{
			public:
				void	Acquire(BDataReference*& ref);
				void	Release(BDataReference*& ref);

				char*	Data()			{ return fData; }
				size_t	Size() const	{ return fSize; }
				int32	Count()			{ return fCount; }

				static void	Create(const void* data, size_t len,
								   BDataReference*& ref, bool copy = false);
				static void Create(size_t len, BDataReference*& ref);

			private:
				BDataReference(const void* data, size_t len, bool copy = false);
				BDataReference(size_t len);
				~BDataReference();

				char*	fData;
				size_t	fSize;
				int32	fCount;
		};

		BDataBuffer();	// No default construction allowed!

		BDataReference*	fDataRef;
};

}	// namespace BPrivate

#endif	// DATABUFFER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

