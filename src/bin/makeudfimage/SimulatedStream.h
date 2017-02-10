//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file SimulatedStream.h
*/

#ifndef _SIMULATED_STREAM_H
#define _SIMULATED_STREAM_H

#include "DataStream.h"

/*! \brief Abstract DataStream wrapper around another DataStream and a sequence of
	extents in said stream that allows for easy write access to said sequence
	of extents as though they were a continuous chunk of data.
	
	NOTE: The SimulatedStream object never modifies the data stream position
	of the underlying data stream; all read/write/zero calls use the underlying
	stream's ReadAt()/WriteAt()/ZeroAt() functions.
*/
class SimulatedStream : public DataStream {
public:
	SimulatedStream(DataStream &stream);
	virtual status_t InitCheck() const;

	virtual ssize_t Read(void *buffer, size_t size);
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);

	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t pos, const void *buffer, size_t size);

	virtual ssize_t Write(BDataIO &data, size_t size);
	virtual ssize_t	WriteAt(off_t pos, BDataIO &data, size_t size);

	virtual ssize_t Zero(size_t size);
	virtual ssize_t	ZeroAt(off_t pos, size_t size);	

	virtual off_t Seek(off_t pos, uint32 seek_mode);
	virtual off_t Position() const { return fPosition; }

	virtual status_t SetSize(off_t size) { return B_ERROR; }

protected:
	struct data_extent {
		data_extent(off_t offset = 0, size_t size = 0)
			: offset(offset)
			, size(size)
		{
		}
		
		off_t offset;
		size_t size;
	};	

	/*! \brief Should be implemented to return (via the output parameter
		\a extent) the largest extent in the underlying data stream corresponding
		to the extent in the simulated data stream starting at byte position
		\a pos of byte length \a size. 
		
		NOTE: If the position is at or beyond the end of the simulated stream, the
		function should return B_OK, and the value of extent.size should be 0.
	*/
	virtual status_t _GetExtent(off_t pos, size_t size, data_extent &extent) = 0;
	
	/*! \brief Should be implemented to return the current size of the stream.
	*/
	virtual off_t _Size() = 0;
	
private:
	off_t fPosition;
	DataStream &fStream;
};

#endif	// _SIMULATED_STREAM_H
