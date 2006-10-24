/*
 *	Copyright (c) 2001-2005, Haiku
 *	Distributed under the terms of the MIT license
 *	Authors:
 			Stefano Ceccherini (burton666@libero.it)
 */
 
// A buffered adapter for BPositionIO objects.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BufferIO.h>

/*! \class BBufferIO
	\brief A buffered adapter for BPositionIO objects.
	\author <a href='mailto:burton666@freemail.it>Stefano Ceccherini</a>
*/


/*! \brief Initializes a BBufferIO object.
	
	Initializes the object, creates a buffer of the given size
	and associates the object with the given BPositionIO stream.

	\param stream A pointer to a BPositionIO object.
	\param buf_size The size of the buffer that the object will allocate and use.
	\param owns_stream Specifies if the object will delete the stream on destruction.
*/
BBufferIO::BBufferIO(BPositionIO *stream, size_t buf_size, bool owns_stream)
	:m_buffer_start(0),
	m_stream(stream),
	m_buffer(NULL),
	m_buffer_used(0),
	m_buffer_dirty(false),
	m_owns_stream(owns_stream)
		
{
	if (buf_size < 512)
		buf_size = 512;
	
	m_buffer_phys = buf_size;
	 
	// What can we do if this malloc fails ?
	// I think R5 uses new, but doesn't catch the thrown exception
	// (if you specify a very big buffer, the application just
	// terminates with abort).  
	m_buffer = (char *)malloc(m_buffer_phys * sizeof(char));
}


/*! \brief Frees the resources allocated by the object
	
	Flushes pending changes to the stream and frees the allocated memory.
	If the owns_stream property is true, the destructor also
	deletes the stream associated with the BBufferIO object.
*/
BBufferIO::~BBufferIO()
{
	if (m_buffer_dirty)
		Flush(); // Writes pending changes to the stream

	free(m_buffer);
	
	if (m_owns_stream)
		delete m_stream;
}


/*! \brief Reads the specified amount of bytes at the given position.
	\param pos The offset into the stream where to read.
	\param buffer A pointer to a buffer where to copy the read data.
	\param size The amount of bytes to read.
	\return The amount of bytes actually read, or an error code.
*/
ssize_t
BBufferIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	// We refuse to crash, even if
	// you were lazy and didn't give a valid
	// stream on construction.
	if (m_stream == NULL)	
		return B_NO_INIT; 
	
	if (buffer == NULL)
		return B_BAD_VALUE;
						
	// If the amount of data we want doesn't fit in the buffer, just
	// read it directly from the disk (and don't touch the buffer).
	if (size > m_buffer_phys) {
		if (m_buffer_dirty)
			Flush();
		return m_stream->ReadAt(pos, buffer, size);
	}
				
	// If the data we are looking for is not in the buffer... 
	if (size > m_buffer_used 
		|| pos < m_buffer_start
		|| pos > m_buffer_start + m_buffer_used
		|| pos + size > m_buffer_start + m_buffer_used) {
		
		if (m_buffer_dirty)
			Flush(); // If there are pending writes, do them.
	
		// ...cache as much as we can from the stream
		m_buffer_used = m_stream->ReadAt(pos, m_buffer, m_buffer_phys);
					
		if (m_buffer_used > 0)
			m_buffer_start = pos; // The data is buffered starting from this offset
	}
	 
	size = min_c(size, m_buffer_used);	
	
	// copy data from the cache to the given buffer
	memcpy(buffer, m_buffer + pos - m_buffer_start, size);
	
	return size;
}


/*! \brief Writes the specified amount of bytes at the given position.
	\param pos The offset into the stream where to write.
	\param buffer A pointer to a buffer which contains the data to write.
	\param size The amount of bytes to write.
	\return The amount of bytes actually written, or an error code.
*/
ssize_t
BBufferIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (m_stream == NULL)
		return B_NO_INIT;
	
	if (buffer == NULL)
		return B_BAD_VALUE;
	
	// If data doesn't fit into the buffer, write it directly to the stream	
	if (size > m_buffer_phys)
		return m_stream->WriteAt(pos, buffer, size);
	
	// If we have cached data in the buffer, whose offset into the stream
	// is > 0, and the buffer isn't dirty, drop the data.
	if (!m_buffer_dirty && m_buffer_start > pos) {
		m_buffer_start = 0;
		m_buffer_used = 0;
	}	
	
	// If we want to write beyond the cached data...
	if (pos > m_buffer_start + m_buffer_used
		|| pos < m_buffer_start) {
		ssize_t read;
		off_t where = pos;
		
		if (pos + size <= m_buffer_phys) // Can we just cache from the beginning ?
			where = 0;
		
		// ...cache more.
		read = m_stream->ReadAt(where, m_buffer, m_buffer_phys);
		if (read > 0) {
			m_buffer_used = read;
			m_buffer_start = where;
		}
	}
	
	memcpy(m_buffer + pos - m_buffer_start, buffer, size); 	
	
	m_buffer_dirty = true;
	m_buffer_used = max_c((size + pos), m_buffer_used);
	
	return size;
}


/*! \brief Sets the position in the stream.
	
	 Sets the position in the stream where the Read() and Write() functions
	 (inherited from BPositionIO) begin reading and writing.
	 How the position argument is understood depends on the seek_mode flag.
	 
	 \param position The position where you want to seek.
	 \param seek_mode Can have three values:

	 - \c SEEK_SET. The position passed is an offset from the beginning of the stream;
		in other words, the current position is set to position.
		For this mode, position should be a positive value. 
	 
	 - \c SEEK_CUR. The position argument is an offset from the current position;
		the value of the argument is added to the current position.
		
	 - \c SEEK_END. The position argument is an offset from the end of the stream.
		In this mode the position argument should be negative (or zero).
	
	 \return The current position as an offset in bytes
		from the beginning of the stream.
*/
off_t
BBufferIO::Seek(off_t position, uint32 seek_mode)
{
	if (m_stream == NULL)	
		return B_NO_INIT; 	
							
	return m_stream->Seek(position, seek_mode);
}


/*! \brief Return the current position in the stream.
	\return The current position as an offset in bytes
		from the beginning of the stream.
*/
off_t
BBufferIO::Position() const
{
	if (m_stream == NULL)	
		return B_NO_INIT;
							
	return m_stream->Position();
}


/*! \brief Calls the SetSize() function of the assigned BPositionIO object.
	\param size The new size of the BPositionIO object.
	\return An error code.
*/
status_t
BBufferIO::SetSize(off_t size)
{
	if (m_stream == NULL)	
		return B_NO_INIT;

	return m_stream->SetSize(size);
}


/*! \brief Writes pending modifications to the stream.
	\return An error code.
*/
status_t
BBufferIO::Flush()
{
	if (!m_buffer_dirty)
		return B_OK;
		
	// Write the cached data to the stream
	status_t err = m_stream->WriteAt(m_buffer_start, m_buffer, m_buffer_used);
	
	if (err > 0)
		m_buffer_dirty = false;
	
	return (err < 0) ? err : B_OK;
}


/*! \brief Returns a pointer to the stream specified on construction
	\return A pointer to the BPositionIO stream specified on construction.
*/
BPositionIO *
BBufferIO::Stream() const
{
	return m_stream;
}


/*! \brief Returns the size of the internal buffer 
	\return The size of the buffer allocated by the object
*/
size_t
BBufferIO::BufferSize() const
{
	return m_buffer_phys;
}


/*! \brief Tells if the BBufferIO object "owns" the specified stream.
	\return A boolean value, which is true if the object "owns"
		the stream (and so will delete it upon destruction), false if not.
*/
bool
BBufferIO::OwnsStream() const
{
	return m_owns_stream;
}


/*! \brief Set the "owns_stream" property of the object.
	\param owns_stream If it's true, the object will delete the stream
		upon destruction, if it's false it will not.
*/
void
BBufferIO::SetOwnsStream(bool owns_stream)
{
	m_owns_stream = owns_stream;
}


/*! \brief Prints the object to stdout.
*/
void
BBufferIO::PrintToStream() const
{
	printf("stream %p\n", m_stream);
	printf("buffer %p\n", m_buffer);
	printf("start  %lld\n", m_buffer_start);
	printf("used   %u\n", m_buffer_used);
	printf("phys   %u\n", m_buffer_phys);
	printf("dirty  %s\n", (m_buffer_dirty) ? "true" : "false");
	printf("owns   %s\n", (m_owns_stream) ? "true" : "false");
}


// 	These functions are here to maintain future binary
// 	compatibility.	 
status_t BBufferIO::_Reserved_BufferIO_0(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_1(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_2(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_3(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_4(void *) { return B_ERROR; }
