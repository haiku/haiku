// OffsetFile.h

#ifndef OFFSET_FILE_H
#define OFFSET_FILE_H

#include <DataIO.h>
#include <File.h>

class OffsetFile : public BPositionIO {
public:
								OffsetFile();
								OffsetFile(const BFile& file, off_t offset);
	virtual						~OffsetFile();

			status_t			SetTo(const BFile& file, off_t offset);
			void				Unset();
			status_t			InitCheck() const;

//			ssize_t				Read(void *buffer, size_t size);
//			ssize_t				Write(const void *buffer, size_t size);
			ssize_t				ReadAt(off_t pos, void *buffer, size_t size);
			ssize_t				WriteAt(off_t pos, const void *buffer,
										size_t size);
			off_t				Seek(off_t position, uint32 seekMode);
			off_t				Position() const;
			status_t			SetSize(off_t size);
			status_t			GetSize(off_t* size);

			off_t				GetOffset() const;

private:
			BFile				fFile;
			off_t				fOffset;
			off_t				fCurrentPosition;
};

#endif	// OFFSET_FILE_H
