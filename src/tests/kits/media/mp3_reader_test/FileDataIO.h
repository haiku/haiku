#ifndef _FILE_DATA_IO
#define _FILE_DATA_IO

#include <DataIO.h>
#include <File.h>

class FileDataIO : public BDataIO
{
public:
	FileDataIO(const char *filepath, uint32 open_mode);
	~FileDataIO();

	ssize_t Read(void *buffer, size_t size);
	ssize_t Write(const void *buffer, size_t size);

private:
	BFile *file;
};

#endif
