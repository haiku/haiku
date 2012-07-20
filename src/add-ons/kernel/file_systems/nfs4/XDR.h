/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef XDR_H
#define XDR_H


#include <SupportDefs.h>


namespace XDR {

class Stream {
public:
	typedef	uint32			Position;

	virtual					~Stream();

	inline	const void*		Buffer() const;
	inline	Position		Current() const;

protected:
							Stream(void* buffer, uint32 size);

	inline	uint32			_PositionToSize() const;
	inline	uint32			_RealSize(uint32 size) const;

			uint32*			fBuffer;
			uint32			fSize;
			Position		fPosition;
};

class ReadStream : public Stream {
public:
							ReadStream(void* buffer, uint32 size);
	virtual					~ReadStream();

	inline	void			SetPosition(Position position);

	inline	int				Size() const;

			int32			GetInt();
			uint32			GetUInt();
			int64			GetHyper();
			uint64			GetUHyper();

	inline	bool			GetBoolean();

			char*			GetString();

			const void*		GetOpaque(uint32* size);

	inline	bool			IsEOF() const;

private:
			bool			fEOF;
};

class WriteStream : public Stream {
public:
							WriteStream();
							WriteStream(const WriteStream& x);
	virtual					~WriteStream();

	inline	int				Size() const;
			void			Clear();

			status_t		InsertUInt(Stream::Position pos, uint32 x);

			status_t		AddInt(int32 x);
			status_t		AddUInt(uint32 x);
			status_t		AddHyper(int64 x);
			status_t		AddUHyper(uint64 x);

	inline	status_t		AddBoolean(bool x);

			status_t		AddString(const char* str, uint32 maxlen = 0);

			status_t		AddOpaque(const void* ptr, uint32 size);
			status_t		AddOpaque(const WriteStream& stream);

			status_t		Append(const WriteStream& stream);

	inline	status_t		Error() const;

private:
			status_t		_CheckResize(uint32 size);

			status_t		fError;

	static 	const uint32	kInitialSize	= 64;
};


inline const void*
Stream::Buffer() const
{
	return fBuffer;
}


inline Stream::Position
Stream::Current() const
{
	return fPosition;
}


inline void
ReadStream::SetPosition(Position position)
{
	fPosition = position;
}


inline int
ReadStream::Size() const
{
	return fSize;
}


inline bool
ReadStream::IsEOF() const
{
	return fEOF;
}


inline bool
ReadStream::GetBoolean()
{
	return GetInt() != 0;
}


inline int
WriteStream::Size() const
{
	return fPosition * sizeof(uint32);
}


inline status_t
WriteStream::AddBoolean(bool x)
{
	return AddInt(static_cast<int32>(x));
}


inline status_t
WriteStream::Error() const
{
	return fError;
}


}		// namespace XDR


#endif	// XDR_H

