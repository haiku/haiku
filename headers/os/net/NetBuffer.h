/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETBUFFER_H
#define _NETBUFFER_H


#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>

#include <sys/socket.h>


class BNetBuffer : public BArchivable {
	public:
		BNetBuffer(size_t size = 0);
		virtual ~BNetBuffer();

		status_t InitCheck();

		BNetBuffer(BMessage* archive);
		virtual status_t Archive(BMessage* into, bool deep = true) const;
		static BArchivable* Instantiate(BMessage* archive);

		BNetBuffer(const BNetBuffer &);
		BNetBuffer& operator=(const BNetBuffer &);

		// Mutators.
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

		// Accessors.
		unsigned char* Data() const;
		size_t Size() const;
		size_t BytesRemaining() const;

	private:
		virtual void _ReservedBNetBufferFBCCruft1();
		virtual void _ReservedBNetBufferFBCCruft2();
		virtual void _ReservedBNetBufferFBCCruft3();
		virtual void _ReservedBNetBufferFBCCruft4();
		virtual void _ReservedBNetBufferFBCCruft5();
		virtual void _ReservedBNetBufferFBCCruft6();

		// Private class helpers.
		status_t clone(const BNetBuffer&);
		status_t dpop(int32, int32, void*);
		status_t dpush(int32, int32, const void*);
		status_t resize(int32 newSize, bool regenStack = false);
		
		status_t	fInit;		
		uint8*		fData;
		int32		fDataSize;
		int32		fStackSize;
		int32		fCapacity;
		int32		_reserved[5];
};

#endif	// _NETBUFFER_H
