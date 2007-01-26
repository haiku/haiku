/*
 *	Copyright (c) 2001-2007, Haiku
 *	Distributed under the terms of the MIT license
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

//! A buffered adapter for BPositionIO objects.


#include <BufferIO.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
BBufferIO::BBufferIO(BPositionIO *stream, size_t bufferSize, bool ownsStream)
	:
	fBufferStart(0),
	fStream(stream),
	fBuffer(NULL),
	fBufferUsed(0),
	fBufferIsDirty(false),
	fOwnsStream(ownsStream)
		
{
	if (bufferSize < 512)
		bufferSize = 512;
	
	fBufferSize = bufferSize;
	 
	// What can we do if this malloc fails ?
	// I think R5 uses new, but doesn't catch the thrown exception
	// (if you specify a very big buffer, the application just
	// terminates with abort).
	fBuffer = (char *)malloc(fBufferSize);
}


/*! \brief Frees the resources allocated by the object

	Flushes pending changes to the stream and frees the allocated memory.
	If the owns_stream property is true, the destructor also
	deletes the stream associated with the BBufferIO object.
*/
BBufferIO::~BBufferIO()
{
	if (fBufferIsDirty) {
		// Write pending changes to the stream
		Flush();
	}

	free(fBuffer);

	if (fOwnsStream)
		delete fStream;
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
	if (fStream == NULL)	
		return B_NO_INIT; 

	if (buffer == NULL)
		return B_BAD_VALUE;

	// If the amount of data we want doesn't fit in the buffer, just
	// read it directly from the disk (and don't touch the buffer).
	if (size > fBufferSize) {
		if (fBufferIsDirty)
			Flush();
		return fStream->ReadAt(pos, buffer, size);
	}

	// If the data we are looking for is not in the buffer... 
	if (size > fBufferUsed 
		|| pos < fBufferStart
		|| pos > fBufferStart + fBufferUsed
		|| pos + size > fBufferStart + fBufferUsed) {
		if (fBufferIsDirty)
			Flush(); // If there are pending writes, do them.

		// ...cache as much as we can from the stream
		fBufferUsed = fStream->ReadAt(pos, fBuffer, fBufferSize);

		if (fBufferUsed > 0)
			fBufferStart = pos; // The data is buffered starting from this offset
	}

	size = min_c(size, fBufferUsed);	

	// copy data from the cache to the given buffer
	memcpy(buffer, fBuffer + pos - fBufferStart, size);

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
	if (fStream == NULL)
		return B_NO_INIT;

	if (buffer == NULL)
		return B_BAD_VALUE;

	// If data doesn't fit into the buffer, write it directly to the stream	
	if (size > fBufferSize)
		return fStream->WriteAt(pos, buffer, size);

	// If we have cached data in the buffer, whose offset into the stream
	// is > 0, and the buffer isn't dirty, drop the data.
	if (!fBufferIsDirty && fBufferStart > pos) {
		fBufferStart = 0;
		fBufferUsed = 0;
	}	

	// If we want to write beyond the cached data...
	if (pos > fBufferStart + fBufferUsed
		|| pos < fBufferStart) {
		ssize_t read;
		off_t where = pos;

		if (pos + size <= fBufferSize) // Can we just cache from the beginning ?
			where = 0;

		// ...cache more.
		read = fStream->ReadAt(where, fBuffer, fBufferSize);
		if (read > 0) {
			fBufferUsed = read;
			fBufferStart = where;
		}
	}

	memcpy(fBuffer + pos - fBufferStart, buffer, size); 	

	fBufferIsDirty = true;
	fBufferUsed = max_c((size + pos), fBufferUsed);

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
BBufferIO::Seek(off_t position, uint32 seekMode)
{
	if (fStream == NULL)
		return B_NO_INIT;

	return fStream->Seek(position, seekMode);
}


/*! \brief Return the current position in the stream.
	\return The current position as an offset in bytes
		from the beginning of the stream.
*/
off_t
BBufferIO::Position() const
{
	if (fStream == NULL)
		return B_NO_INIT;

	return fStream->Position();
}


/*! \brief Calls the SetSize() function of the assigned BPositionIO object.
	\param size The new size of the BPositionIO object.
	\return An error code.
*/
status_t
BBufferIO::SetSize(off_t size)
{
	if (fStream == NULL)
		return B_NO_INIT;

	return fStream->SetSize(size);
}


/*! \brief Writes pending modifications to the stream.
	\return An error code.
*/
status_t
BBufferIO::Flush()
{
	if (!fBufferIsDirty)
		return B_OK;

	// Write the cached data to the stream
	ssize_t bytesWritten = fStream->WriteAt(fBufferStart, fBuffer, fBufferUsed);
	if (bytesWritten > 0)
		fBufferIsDirty = false;

	return (bytesWritten < 0) ? bytesWritten : B_OK;
}


/*! \brief Returns a pointer to the stream specified on construction
	\return A pointer to the BPositionIO stream specified on construction.
*/
BPositionIO *
BBufferIO::Stream() const
{
	return fStream;
}


/*! \brief Returns the size of the internal buffer 
	\return The size of the buffer allocated by the object
*/
size_t
BBufferIO::BufferSize() const
{
	return fBufferSize;
}


/*! \brief Tells if the BBufferIO object "owns" the specified stream.
	\return A boolean value, which is true if the object "owns"
		the stream (and so will delete it upon destruction), false if not.
*/
bool
BBufferIO::OwnsStream() const
{
	return fOwnsStream;
}


/*! \brief Set the "owns_stream" property of the object.
	\param owns_stream If it's true, the object will delete the stream
		upon destruction, if it's false it will not.
*/
void
BBufferIO::SetOwnsStream(bool owns_stream)
{
	fOwnsStream = owns_stream;
}


/*! \brief Prints the object to stdout.
*/
void
BBufferIO::PrintToStream() const
{
	printf("stream %p\n", fStream);
	printf("buffer %p\n", fBuffer);
	printf("start  %lld\n", fBufferStart);
	printf("used   %ld\n", fBufferUsed);
	printf("phys   %ld\n", fBufferSize);
	printf("dirty  %s\n", (fBufferIsDirty) ? "true" : "false");
	printf("owns   %s\n", (fOwnsStream) ? "true" : "false");
}


//	#pragma mark -


// These functions are here to maintain future binary
// compatibility.	 
status_t BBufferIO::_Reserved_BufferIO_0(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_1(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_2(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_3(void *) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_4(void *) { return B_ERROR; }
