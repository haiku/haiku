/******************************************************************************
/
/	File:			BufferIO.h
/
/	Description:	A buffered adapter for BPositionIO
/
/	Copyright 1998, Be Incorporated
/
******************************************************************************/

#if !defined(_BUFFER_IO_H)
#define _BUFFER_IO_H

#include <DataIO.h>

class BBufferIO : public BPositionIO
{
public:
	enum {
		DEFAULT_BUF_SIZE = 65536L
	};

		BBufferIO(
				BPositionIO * stream,
				size_t buf_size = DEFAULT_BUF_SIZE,
				bool owns_stream = true);
virtual		~BBufferIO();

virtual		ssize_t ReadAt(
				off_t pos, 
				void *buffer, 
				size_t size);
virtual		ssize_t WriteAt(
				off_t pos, 
				const void *buffer, 
				size_t size);
virtual		off_t Seek(
				off_t position, 
				uint32 seek_mode);
virtual		off_t Position() const;
virtual		status_t SetSize(
				off_t size);
virtual		status_t Flush();
		BPositionIO * Stream() const;
		size_t BufferSize() const;
		bool OwnsStream() const;
		void SetOwnsStream(
				bool owns_stream);

		void PrintToStream() const;

private:

		off_t m_buffer_start;
		BPositionIO * m_stream;
		char * m_buffer;
		size_t m_buffer_phys;
		size_t m_buffer_used;
		uint32 _reserved_ints[6];
		bool m_buffer_dirty;
		bool m_owns_stream;
		bool _reserved_bools[6];

virtual		status_t _Reserved_BufferIO_0(void *);
virtual		status_t _Reserved_BufferIO_1(void *);
virtual		status_t _Reserved_BufferIO_2(void *);
virtual		status_t _Reserved_BufferIO_3(void *);
virtual		status_t _Reserved_BufferIO_4(void *);

};


#endif /* _BUFFER_IO_H */
