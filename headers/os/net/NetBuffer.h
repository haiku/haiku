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
//------------------------------------------------------------------------------

#ifndef _NETBUFFER_H
#define _NETBUFFER_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>
#include <sys/socket.h>


/*
 * BNetBuffer
 *
 * BNetBuffer is a dynamic buffer useful for storing data to be sent
 * across the network. Data is inserted into and removed from 
 * the object using one of the many insert and remove member functions.
 * Access to the raw stored data is possible. The BNetEndpoint class has a
 * send and recv function for use with BNetBuffer. Network byte order conversion
 * is done automatically for all appropriate integral types in the insert and remove
 * functions for that type.
 *
 */

class NLPacket;

class BNetBuffer : public BArchivable
{
public:

	BNetBuffer(size_t size = 0);
	virtual ~BNetBuffer();

	/*
	 * It is important to call InitCheck() after creating an instance of this class.
	 */	

	status_t InitCheck();
	
	BNetBuffer(BMessage *archive);
	virtual	status_t Archive(BMessage *into, bool deep = true) const;
	static BArchivable *Instantiate(BMessage *archive);

	BNetBuffer(const BNetBuffer &);
	BNetBuffer &operator=(const BNetBuffer &);
	

	/*
	 *  Data insertion member functions.  Data is inserted at the end of the internal 
	 *  dynamic buffer.  For the short, int, and long versions (and unsigned of 
	 *  each as well) byte order conversion is performed.
	 *
	 *  The terminating null of all strings are inserted as 
	 *  well.
	 *
	 *  Returns B_OK if successful, B_ERROR otherwise.
	 */
	status_t AppendInt8(int8);
	status_t AppendUint8(uint8);
	status_t AppendInt16(int16);
	status_t AppendUint16(uint16);
	status_t AppendInt32(int32);
	status_t AppendUint32(uint32);
	status_t AppendFloat(float);
	status_t AppendDouble(double);
	status_t AppendString(const char *);
	status_t AppendData(const void *, size_t);
	status_t AppendMessage(const BMessage &);
	status_t AppendInt64(int64);
	status_t AppendUint64(uint64);

	/*
	 *  Data extraction member functions.  Data is extracted from the start of the 
	 *  internal dynamic buffer.  For the short, int, and long versions (and 
	 *  unsigned of each as well) byte order conversion is performed.
	 *
	 *  Returns B_OK if successful, B_ERROR otherwise.
	 */
 	status_t RemoveInt8(int8 &);
	status_t RemoveUint8(uint8 &);
	status_t RemoveInt16(int16 &);
	status_t RemoveUint16(uint16 &);
	status_t RemoveInt32(int32 &);
	status_t RemoveUint32(uint32 &);
	status_t RemoveFloat(float &);
	status_t RemoveDouble(double &);
	status_t RemoveString(char *, size_t);
	status_t RemoveData(void *, size_t);
	status_t RemoveMessage(BMessage &);
	status_t RemoveInt64(int64 &);
	status_t RemoveUint64(uint64 &);
 
	/*
	 * GetData() returns a pointer to the internal data buffer.  This is useful 
	 * when wanting to pass the data to a function expecting a pointer to data. 
	 * GetSize() returns the size of the BNetPacket's data, in bytes. 
	 * GetBytesRemaining() returns the bytes remaining in the BNetPacket available 
	 * to be extracted via BNetPacket::Remove*().
	 */
	unsigned char *Data() const;
	size_t Size() const;
	size_t BytesRemaining() const;
	
	inline NLPacket *GetImpl() const;

protected:
	status_t m_init;

private:
	virtual	void		_ReservedBNetBufferFBCCruft1();
	virtual	void		_ReservedBNetBufferFBCCruft2();
	virtual	void		_ReservedBNetBufferFBCCruft3();
	virtual	void		_ReservedBNetBufferFBCCruft4();
	virtual	void		_ReservedBNetBufferFBCCruft5();
	virtual	void		_ReservedBNetBufferFBCCruft6();

	int32				__ReservedBNetBufferFBCCruftData[9];

};


inline NLPacket *BNetBuffer::GetImpl() const
{
	return NULL;	// No Nettle backward compatibility
}

#endif	// _NETBUFFER_H

