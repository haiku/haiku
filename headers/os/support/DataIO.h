// Modified BeOS header. Just here to be able to compile and test BFile.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#ifndef __sk_data_io_h__
#define __sk_data_io_h__

#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

/*-----------------------------------------------------------------*/
/*------- BDataIO Class -------------------------------------------*/

class BDataIO {
public:
					BDataIO();
virtual				~BDataIO();

virtual	ssize_t		Read(void *buffer, size_t size) = 0;
virtual	ssize_t		Write(const void *buffer, size_t size) =0;

/*----- Private or reserved ---------------*/
private:

virtual	void		_ReservedDataIO1();
virtual	void		_ReservedDataIO2();
virtual	void		_ReservedDataIO3();
virtual	void		_ReservedDataIO4();

					BDataIO(const BDataIO &);
		BDataIO		&operator=(const BDataIO &);

		int32		_reserved[2];
};

/*---------------------------------------------------------------------*/
/*------- BPositionIO Class -------------------------------------------*/

class BPositionIO : public BDataIO {
public:
					BPositionIO();
virtual				~BPositionIO();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual	ssize_t		Write(const void *buffer, size_t size);

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size) = 0;
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size) = 0;

virtual off_t		Seek(off_t position, uint32 seek_mode) = 0;
virtual	off_t		Position() const = 0;

virtual status_t	SetSize(off_t size);

/*----- Private or reserved ---------------*/
private:
virtual	void		_ReservedPositionIO1();
virtual	void		_ReservedPositionIO2();
virtual void		_ReservedPositionIO3();
virtual void		_ReservedPositionIO4();

		int32		_reserved[2];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif	// __sk_data_io_h__
