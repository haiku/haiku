/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FD_CLOSER_H
#define FD_CLOSER_H


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


#endif	// FD_CLOSER_H
