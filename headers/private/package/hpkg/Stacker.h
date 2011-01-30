/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__STACKER_H_
#define _PACKAGE__HPKG__PRIVATE__STACKER_H_


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


template<typename Type>
class Stacker {
public:
	Stacker(Type*& location, Type* element)
		:
		fLocation(&location),
		fPreviousElement(location)
	{
		*fLocation = element;
	}

	Stacker(Type** location, Type* element)
		:
		fLocation(location),
		fPreviousElement(*location)
	{
		*fLocation = element;
	}

	~Stacker()
	{
		if (fLocation != NULL)
			*fLocation = fPreviousElement;
	}

private:
	Type**	fLocation;
	Type*	fPreviousElement;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__STACKER_H_
