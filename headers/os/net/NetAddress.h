/*=--------------------------------------------------------------------------=*
 * NetAddress.h -- Interface definition for the BNetAddress class.
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

#ifndef _NETADDRESS_H
#define _NETADDRESS_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>
#include <socket.h>

/*
 * Empty forward declaration to satisfy the GetImpl method.
 */
class NLAddress;

class BNetAddress : public BArchivable
{
public:
    BNetAddress( BMessage* archive );
    virtual ~BNetAddress();

    virtual  status_t Archive( BMessage* into, bool deep = true ) const;
    static   BArchivable* Instantiate( BMessage* archive );

    BNetAddress( const char* hostname = 0, unsigned short port = 0 );
    BNetAddress( const struct sockaddr_in& sa );
    BNetAddress( in_addr addr, int port = 0 );
    BNetAddress( uint32 addr, int port = 0 );
    BNetAddress( const BNetAddress& refparam );
    BNetAddress( const char* hostname, const char* protocol, const char* service );

    BNetAddress& operator=( const BNetAddress& );

    status_t InitCheck();

    status_t SetTo( const char* hostname, const char* protocol, const char* service );
    status_t SetTo( const char* hostname = NULL, unsigned short port = 0 );
    status_t SetTo( const struct sockaddr_in& sa );
    status_t SetTo( in_addr addr, int port = 0 );
    status_t SetTo( uint32 addr = INADDR_ANY, int port = 0 );

    status_t GetAddr( char* hostname = NULL, unsigned short* port = NULL ) const;
    status_t GetAddr( struct sockaddr_in& sa ) const;
    status_t GetAddr( in_addr& addr, unsigned short* port = NULL ) const;

    inline NLAddress *GetImpl() const;

protected:
    status_t m_init;

private:
    virtual void _ReservedBNetAddressFBCCruft1();
    virtual void _ReservedBNetAddressFBCCruft2();
    virtual void _ReservedBNetAddressFBCCruft3();
    virtual void _ReservedBNetAddressFBCCruft4();
    virtual void _ReservedBNetAddressFBCCruft5();
    virtual void _ReservedBNetAddressFBCCruft6();

// Private copy helper.
    status_t clone( const BNetAddress& RefParam );

    int32    fFamily;   // Encapsulation of sockaddr::sin_family
    int32    fPort;     // Encapsulation of sockaddr::sin_port
    int32    fAddress;  // Encapsulation of sockaddr::sin_addr.s_addr
    
// Not used, here to maintain R5 binary compatiblilty.
    int32   fPrivateData[6];
};


inline NLAddress* BNetAddress::GetImpl( void ) const
{
	return NULL;	// No Nettle backward compatibility
}

#endif // <-- #ifndef _NETADDRESS_H


/*=------------------------------------------------------------------- End -=*/

