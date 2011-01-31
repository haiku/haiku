/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__FD_CLOSER_H_
#define _PACKAGE__HPKG__PRIVATE__FD_CLOSER_H_


namespace BPackageKit {

namespace BHPKG {

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

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__FD_CLOSER_H_
