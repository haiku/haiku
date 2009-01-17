/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */

#ifndef _NETBUFFER_H
#define _NETBUFFER_H

#include <Archivable.h>
#include <SupportDefs.h>


class BMessage;
class DynamicBuffer;


class BNetBuffer : public BArchivable {
public:
	BNetBuffer(size_t size = 0);

	virtual ~BNetBuffer();

	BNetBuffer(const BNetBuffer& buffer);	
	BNetBuffer(BMessage* archive);

	BNetBuffer& operator=(const BNetBuffer& buffer);

	virtual	status_t Archive(BMessage* into, bool deep = true) const;
	static BArchivable* Instantiate(BMessage* archive);	

	status_t InitCheck();

	status_t AppendInt8(int8 data);
	status_t AppendUint8(uint8 data);
	status_t AppendInt16(int16 data);
	status_t AppendUint16(uint16 data);
	status_t AppendInt32(int32 data);
	status_t AppendUint32(uint32 data);
	status_t AppendFloat(float data);
	status_t AppendDouble(double data);
	status_t AppendString(const char* data);
	status_t AppendData(const void* data, size_t size);
	status_t AppendMessage(const BMessage& data);
	status_t AppendInt64(int64 data);
	status_t AppendUint64(uint64 data);

 	status_t RemoveInt8(int8& data);
	status_t RemoveUint8(uint8& data);
	status_t RemoveInt16(int16& data);
	status_t RemoveUint16(uint16& data);
	status_t RemoveInt32(int32& data);
	status_t RemoveUint32(uint32& data);
	status_t RemoveFloat(float& data);
	status_t RemoveDouble(double& data);
	status_t RemoveString(char* data, size_t size);
	status_t RemoveData(void* data, size_t size);
	status_t RemoveMessage(BMessage& data);
	status_t RemoveInt64(int64& data);
	status_t RemoveUint64(uint64& data);
 
	unsigned char* Data() const;
	size_t Size() const;
	size_t BytesRemaining() const;
	
	inline DynamicBuffer* GetImpl() const;

protected:
	status_t fInit;

private:
	virtual	void		_ReservedBNetBufferFBCCruft1();
	virtual	void		_ReservedBNetBufferFBCCruft2();
	virtual	void		_ReservedBNetBufferFBCCruft3();
	virtual	void		_ReservedBNetBufferFBCCruft4();
	virtual	void		_ReservedBNetBufferFBCCruft5();
	virtual	void		_ReservedBNetBufferFBCCruft6();

	DynamicBuffer* 		fImpl;
	int32				__ReservedBNetBufferFBCCruftData[8];

};


inline DynamicBuffer*
BNetBuffer::GetImpl() const
{
	return fImpl;
}

#endif  // _NET_BUFFER_H
