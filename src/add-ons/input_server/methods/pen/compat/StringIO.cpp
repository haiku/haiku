/*
 * minimalistic Dano-like BStringIO
 * (c) 2007, François Revol.
 */
#include <BeBuild.h>
#ifndef B_BEOS_VERSION_DANO

#include <StringIO.h>
#include <Rect.h>
#include <unistd.h>
//#include <stdint.h>

// stripped down BStringIO



BStringIO::BStringIO()
{
	fString = new BString;
}

BStringIO::~BStringIO()
{
	delete fString;
}

ssize_t
BStringIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	return EIO;
}

ssize_t
BStringIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (pos > (2147483647L)/*INT32_MAX*/)
		return EINVAL;
	if (fString->Length() < pos)
		fString->Insert(' ', (int32)(pos - fString->Length()), fString->Length());
	fString->Remove((int32)pos, size);
	fString->Insert((const char *)buffer, size, (int32)pos);
	return size;
}

off_t
BStringIO::Seek(off_t pos, uint32 seek_mode)
{
	switch (seek_mode) {
	case SEEK_CUR:
		fPosition += pos;
		return fPosition;
	case SEEK_SET:
		fPosition = pos;
		return fPosition;
	case SEEK_END:
		fPosition = fString->Length() - pos;
		if (fPosition < 0)
			fPosition = 0;
		return fPosition;
	default:
		return EINVAL;
	}
}

off_t
BStringIO::Position() const
{
	return fPosition;
}

status_t
BStringIO::SetSize(off_t size)
{
	return EINVAL;
}

const char *
BStringIO::String() const
{
	return fString->String();
}


// ops
BStringIO & BStringIO::operator<<(const BString & s) {this->BPositionIO::Write(s.String(), s.Length()); return *this;};
BStringIO & BStringIO::operator<<(const BRect & r) {BString s; s << "Rect{" << r.left << r.top << r.right << r.bottom << "}"; this->BPositionIO::Write(s.String(), s.Length()); return *this;};

#endif /* B_BEOS_VERSION_DANO */
