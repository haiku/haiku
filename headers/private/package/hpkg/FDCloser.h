/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FD_CLOSER_H
#define FD_CLOSER_H


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


struct FDCloser {
	FDCloser(int fd)
		:
		fFD(fd)
	{
	}

	~FDCloser()
	{
		if (fFD >= 0)
			close(fFD);
	}

private:
	int	fFD;
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// FD_CLOSER_H
