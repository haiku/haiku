//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		BufferIO.cpp
//	Author(s):		Stefano Ceccherini (burton666@libero.it)
//
//	Description:	A buffered adapter for BPositionIO objects.  
//------------------------------------------------------------------------------


/*	XXX: Currently, this class implement buffered reading, 
	but not (yet) buffered writing.
	I'm doing tests to ensure that the implementation of the buffered
	writing will be correct.
*/

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
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
	  
	m_buffer = (char*)malloc(m_buffer_phys * sizeof(char));
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
	\param buffer A pointer to a buffer where to put the read data.
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
	
	if (m_buffer_dirty)
		Flush(); // If there are pending writes, do them.
				
	// If the amount of data we want doesn't fit in the buffer, just
	// read it directly from the disk.
	if (size > m_buffer_phys)
		return ReadAt(pos, buffer, size);
		
	// If the data we are looking for is not in the buffer... 
	if (size > m_buffer_used 
		|| pos < m_buffer_start
		|| pos > m_buffer_start + m_buffer_used)
	{
		// ...cache as much as we can from the stream
		ssize_t readSize = m_stream->ReadAt(pos, m_buffer, m_buffer_phys);
		m_buffer_used += readSize;
				
		if (readSize > 0)
			m_buffer_start = pos; // The data is buffered starting from this offset
	}
	 
	size = min_c(size, m_buffer_used);	
	
	// copy data from the cache to the given buffer
	memcpy(buffer, m_buffer + pos, size);
	
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
	
	// XXX: Implement buffered write
	
	return m_stream->WriteAt(pos, buffer, size);
}


/*! \brief Sets the position in the stream.
	
	 Sets the position in the stream where the Read() and Write() functions
	 (inherited from BPositionIO) begin reading and writing.
	 How the position argument is understood depends on the seek_mode flag.
	 
	 \param position The position where you want to seek.
	 \param seek_mode Can have three values:

	 - \oSEEK_SET. The position passed is an offset from the beginning of the stream;
		in other words, the current position is set to position.
		For this mode, position should be a positive value. 
	 
	 - \oSEEK_CUR. The position argument is an offset from the current position;
		the value of the argument is added to the current position.
		
	 - \oSEEK_END. The position argument is an offset from the end of the stream.
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
	m_buffer_dirty = false;
	
	// XXX: Not implemented... yet
	return B_ERROR;
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
	printf("used   %ld\n", m_buffer_used);
	printf("phys   %ld\n", m_buffer_phys);
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
