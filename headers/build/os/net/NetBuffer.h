/*=--------------------------------------------------------------------------=*
 * NetBuffer.h -- Interface definition for the BNetBuffer class.
 *
 * Written by S.T. Mansfield (thephantom@mac.com)
 *=--------------------------------------------------------------------------=*
 * Copyright (c) 2002, The OpenBeOS project.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *=--------------------------------------------------------------------------=*
 */

#ifndef _NETBUFFER_H
#define _NETBUFFER_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>
#include <socket.h>

/*
 * Empty forward declaration to satisfy the GetImpl method.
 */
class NLPacket;

class BNetBuffer : public BArchivable
{
public:
    BNetBuffer(size_t size = 0);
    virtual ~BNetBuffer();

    status_t InitCheck();

    BNetBuffer(BMessage *archive);
    virtual  status_t Archive(BMessage *into, bool deep = true) const;
    static   BArchivable *Instantiate(BMessage *archive);

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
    unsigned char *Data() const;
    size_t Size() const;
    size_t BytesRemaining() const;

    inline NLPacket *GetImpl() const;

protected:
// Attributes.
    status_t        m_init;
    
private:
    virtual void _ReservedBNetBufferFBCCruft1();
    virtual void _ReservedBNetBufferFBCCruft2();
    virtual void _ReservedBNetBufferFBCCruft3();
    virtual void _ReservedBNetBufferFBCCruft4();
    virtual void _ReservedBNetBufferFBCCruft5();
    virtual void _ReservedBNetBufferFBCCruft6();

    unsigned char*  fData;
    int32           fDataSize;
    int32           fStackSize;
    int32           fCapacity;

// Not used, here to maintain R5 binary compatiblilty.
    int32   fPrivateData[5];

// Private class helpers.
    status_t clone( const BNetBuffer& );
    status_t dpop( int32, int32, void* );
    status_t dpush( int32, int32, const void* );
    status_t resize( int32 NewSize, bool RegenStack = false );
};


inline NLPacket* BNetBuffer::GetImpl( void ) const
{
	return NULL;	// No Nettle backward compatibility
}

#endif // <-- #ifndef _NETBUFFER_H

/*=------------------------------------------------------------------- End -=*/

