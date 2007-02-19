// Path.h

#ifndef NET_FS_PATH_H
#define NET_FS_PATH_H

#include <SupportDefs.h>

class Path {
public:
								Path();
								~Path();

			status_t			SetTo(const char* path,
									const char* leaf = NULL);
			status_t			Append(const char* leaf);

			const char*			GetPath() const;
			int32				GetLength() const;

private:
			status_t			_Resize(int32 minLen);

private:
			char*				fBuffer;
			int32				fBufferSize;
			int32				fLength;
};

#endif NET_FS_PATH_H
