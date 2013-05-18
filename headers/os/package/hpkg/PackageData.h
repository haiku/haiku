/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_DATA_H_
#define _PACKAGE__HPKG__PACKAGE_DATA_H_


#include <package/hpkg/HPKGDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageData {
public:
								BPackageData();

			uint64				Size() const
									{ return fSize; }
			uint64				Offset() const
									{ return fEncodedInline ? 0 : fOffset; }

			bool				IsEncodedInline() const
									{ return fEncodedInline; }
			const uint8*		InlineData() const		{ return fInlineData; }

			void				SetData(uint64 size, uint64 offset);
			void				SetData(uint8 size, const void* data);

private:
			uint64				fSize : 63;
			bool				fEncodedInline : 1;
			union {
				uint64			fOffset;
				uint8			fInlineData[B_HPKG_MAX_INLINE_DATA_SIZE];
			};
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_DATA_H_
