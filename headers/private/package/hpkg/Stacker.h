/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACKER_H
#define STACKER_H


namespace BPackageKit {

namespace BHaikuPackage {

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

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// STACKER_H
