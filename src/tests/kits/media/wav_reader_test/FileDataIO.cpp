#include "FileDataIO.h"

FileDataIO::FileDataIO(const char *filepath, uint32 open_mode)
 : file(new BFile(filepath, open_mode))
{
}

FileDataIO::~FileDataIO()
{
	delete file;
}

ssize_t
FileDataIO::Read(void *buffer, size_t size)
{
	return file->Read(buffer, size);
}

ssize_t
FileDataIO::Write(const void *buffer, size_t size)
{
	return file->Write(buffer, size);
}

