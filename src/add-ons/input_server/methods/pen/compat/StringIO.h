/*
 * minimalistic Dano-ilke BStringIO
 * (c) 2007, François Revol.
 */
#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
#include_next <StringIO.h>
#else

#ifndef _STRING_IO_H
#define _STRING_IO_H

#include <DataIO.h>
#include <Rect.h>
#include <String.h>
//#include <stdint.h>

// stripped down BStringIO

class BStringIO : public BPositionIO {
public:
	BStringIO();
virtual ~BStringIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);

//		void		SetBlockSize(size_t blocksize);

const	char*		String() const;
//		size_t		StringLength() const;

BStringIO & operator<<(const BString & s);
BStringIO & operator<<(const BRect & r);


private:
	off_t fPosition;
	BString *fString;
};



#endif /* _STRING_IO_H */

#endif
