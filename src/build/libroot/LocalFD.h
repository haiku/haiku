/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
// no header guards: must be included at appropriate part of .cpp


class LocalFD {
public:
	LocalFD()
	{
	}

	~LocalFD()
	{
	}

	status_t Init(int fd)
	{
#ifndef BUILDING_FS_SHELL
		Descriptor* descriptor = get_descriptor(fd);
		if (descriptor && !descriptor->IsSystemFD()) {
			// we need to get a path
			fFD = -1;
			return descriptor->GetPath(fPath);
		}
#endif

		fFD = fd;
		fPath = "";
		return B_OK;
	}

	int FD() const
	{
		return fFD;
	}

	const char* Path() const
	{
		return (fFD < 0 ? fPath.c_str() : NULL);
	}

	bool IsSymlink() const
	{
		struct stat st;
		int result;
		if (Path())
			result = lstat(Path(), &st);
		else
			result = fstat(fFD, &st);

		return (result == 0 && S_ISLNK(st.st_mode));
	}

private:
	string	fPath;
	int		fFD;
};
