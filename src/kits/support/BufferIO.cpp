/* Just a skeleton of the BBufferIO class */
/* To be implemented */


#include <BufferIO.h>

BBufferIO::BBufferIO(BPositionIO *stream, size_t buf_size, bool owns_stream)
	:
	m_stream(stream),
	m_buffer(new char[buf_size]),
	m_buffer_phys(buf_size),
	m_owns_stream(owns_stream)
		
{
}


BBufferIO::~BBufferIO()
{
	Flush(); //only guessed
	delete[] m_buffer;
	if (m_owns_stream) //only guessed
		delete m_stream; 
}


ssize_t
BBufferIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	return 0;
}


ssize_t
BBufferIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return 0;
}


off_t
BBufferIO::Seek(off_t position, uint32 seek_mode)
{
	return m_stream->Seek(position, seek_mode);
}


off_t
BBufferIO::Position() const
{
	return m_stream->Position();
}


status_t
BBufferIO::SetSize(off_t size)
{
	return B_ERROR;
}


status_t
BBufferIO::Flush()
{
	return B_ERROR;
}


BPositionIO *
BBufferIO::Stream() const
{
	return m_stream;
}


size_t
BBufferIO::BufferSize() const
{
	return m_buffer_phys //or should we return m_buffer_used?
}


bool
BBufferIO::OwnsStream() const
{
	return m_owns_stream;
}


void
BBufferIO::SetOwnsStream(bool owns_stream)
{
	m_owns_stream = owns_stream;
}


void
BBufferIO::PrintToStream() const
{
}


//FBC
status_t _Reserved_BufferIO_0(void *) {}
status_t _Reserved_BufferIO_1(void *) {}
status_t _Reserved_BufferIO_2(void *) {}
status_t _Reserved_BufferIO_3(void *) {}
status_t _Reserved_BufferIO_4(void *) {}