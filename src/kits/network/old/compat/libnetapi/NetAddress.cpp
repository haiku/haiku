/*=--------------------------------------------------------------------------=*
 * NetAddress.cpp -- Implementation of the BNetAddress class.
 *
 * Written by S.T. Mansfield (thephantom@mac.com)
 *
 * Remarks:
 *     * In all accessors, address and port are converted from network to
 *       host byte order.
 *     * In all mutators, address and port are converted from host to
 *       network byte order.
 *     * No trouts were harmed during the development of this class.
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


#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <Message.h>

#include <ByteOrder.h>
#include <NetAddress.h>

/*
 * AF_INET is 2 or 0 depending on whether or not you use the POSIX
 * headers.  Never never never compare by value, always use the
 * #defined token.
 */
#define CTOR_INIT_LIST \
    BArchivable( ), \
    m_init( B_NO_INIT ), \
    fFamily( AF_INET ), \
    fPort( 0 ), \
    fAddress( INADDR_ANY )


/* BNetAddress
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class constructors that have a direct compliment with one of the SetTo
 *     methods.
 *
 * Input parameters:
 *     Varies, see the method signature description for the corresponding
 *     SetTo() method descriptor.
 */

// This bad boy doubles up as the defalt ctor.
BNetAddress::BNetAddress( const char* hostname, unsigned short port )
            : CTOR_INIT_LIST
{
    m_init = SetTo( hostname, port );
}

BNetAddress::BNetAddress( const struct sockaddr_in& sa )
            : CTOR_INIT_LIST
{
    m_init = SetTo( sa );
}

BNetAddress::BNetAddress( in_addr addr, int port )
            : CTOR_INIT_LIST
{
    m_init = SetTo( addr, port );
}

BNetAddress::BNetAddress( uint32 addr, int port )
            : CTOR_INIT_LIST
{
    m_init = SetTo( addr, port );
}

BNetAddress::BNetAddress( const char* hostname, const char* protocol,
                          const char* service )
            : CTOR_INIT_LIST
{
    m_init = SetTo( hostname, protocol, service );
}


/* BNetAddress
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class copy ctor.
 *
 * Input parameter:
 *     refparam: Instance to copy.
 */
BNetAddress::BNetAddress( const BNetAddress& refparam )
            : CTOR_INIT_LIST
{
    m_init = clone( refparam );
}


/* BNetAddress
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Ctor to instantiate from serialized data (BMessage).
 *
 * Input parameter:
 *     archive      : Serialized object to instantiate from.
 */
BNetAddress::BNetAddress( BMessage* archive )
            : CTOR_INIT_LIST
{
    int8 int8_val;
    int32 int32_val;

    if ( archive->FindInt8( "bnaddr_family", &int8_val ) != B_OK )
    {
        return;
    }
    fFamily = int8_val;

    if ( archive->FindInt8( "bnaddr_port", &int8_val ) != B_OK )
    {
        return;
    }
    fPort = int8_val;

    if ( archive->FindInt32( "bnaddr_addr", &int32_val ) != B_OK )
    {
        return;
    }
    fAddress = int32_val;

    m_init = B_OK;
}


/* operator=
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class' assignment operator.
 *
 * Input parameter:
 *     refparam     : Instance to assign from.
 */
BNetAddress & BNetAddress::operator=( const BNetAddress& refparam )
{
    clone(refparam);
    // TODO: what do return on clone() failure!?
    return *this;
}


/* ~BNetAddress
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class dtor.
 */
BNetAddress::~BNetAddress( void )
{
    fFamily = fPort = fAddress = 0;
    m_init = B_NO_INIT;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor.
 *
 * Output parameters:
 *     hostname     : Host name associated with this instance (default: NULL).
 *                    In this particular implementation, hostname will be an
 *                    ASCII-fied representation of an IP address.
 *     port         : Port number associated with this instance
 *                    (default: NULL).  Will be converted to host byte order
 *                    here, so it is not necessary to call ntohs() after
 *                    calling this method.
 *
 * Returns:
 *     B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *     Hostname and/or port can be NULL; although having them both NULL would
 *     be a pointless waste of CPU cycles.  ;-)
 *
 *     The hostname output parameter can be a variety of things, but in this
 *     method we convert the IP address to a string.  See the relevant
 *     documentation about inet_ntoa() for details.
 *
 *     Make sure hostname is large enough or you will step on someone
 *     else's toes.  (Can you say "buffer overflow exploit" boys and girls?)
 *     You can generally be safe using the MAXHOSTNAMELEN define, which
 *     defaults to 64 bytes--don't forget to leave room for the NULL!
 */
status_t BNetAddress::GetAddr( char* hostname, unsigned short* port ) const
{
    if ( m_init != B_OK )
    {
        return B_NO_INIT;
    }

    char* hnBuff;
    struct in_addr ia;

    ia.s_addr = fAddress;

    if ( port != NULL )
    {
        *port = ( unsigned short )ntohs( fPort );
    }

    if ( hostname != NULL )
    {
        hnBuff = inet_ntoa( ia );
        if ( hnBuff != NULL )
        {
            strcpy( hostname, hnBuff );
        }
    }

    return B_OK;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor.
 *
 * Output parameter:
 *     sa           : sockaddr_in struct to be filled.
 *
 * Returns:
 *     B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *     This method fills in the sin_addr, sin_family, and sin_port fields of
 *     the output parameter, all other fields are untouched so we can work
 *     with both POSIX and non-POSIX versions of said struct.  The port and
 *     address values added to the output parameter are in network byte order.
 */
status_t BNetAddress::GetAddr( struct sockaddr_in& sa ) const
{
    if ( m_init != B_OK )
    {
        return B_NO_INIT;
    }

    sa.sin_family = ( uint8 )fFamily;
    sa.sin_port = ( uint8 )fPort;
    sa.sin_addr.s_addr = ( in_addr_t )fAddress;

    return B_OK;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor.
 *
 * Output parameters:
 *     addr         : in_addr struct to fill.
 *     port         : optional port number to fill.
 *
 * Returns:
 *     B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *     Output parameters will be in network byte order, so it is not
 *     necessary to call htons after calling this method.
 */
status_t BNetAddress::GetAddr( in_addr& addr, unsigned short* port ) const
{
    if ( m_init != B_OK )
    {
        return B_NO_INIT;
    }

    addr.s_addr = fAddress;

    if ( port != NULL )
    {
        *port = fPort;
    }

    return B_OK;
}


/* InitCheck
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Determine whether or not this instance is properly initialized.
 *
 * Returns:
 *     B_OK if this instance is initialized, B_ERROR if not.
 */
status_t BNetAddress::InitCheck( void )
{
    return ( m_init == B_OK ) ? B_OK : B_ERROR;
}


/* Archive
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Serialize this instance into the passed BMessage parameter.
 *
 * Input parameter:
 *     deep         : [ignored] default==true.
 *
 * Output parameter:
 *     into         : BMessage object to serialize into.
 *
 * Returns:
 *     B_OK/BERROR on success/failure.  Returns B_NO_INIT if instance not
 *     properly initialized.
 */
status_t BNetAddress::Archive( BMessage* into, bool deep ) const
{
    if ( m_init != B_OK )
    {
        return B_NO_INIT;
    }

    if ( into->AddInt8( "bnaddr_family", fFamily ) != B_OK )
    {
        return B_ERROR;
    }

    if ( into->AddInt8( "bnaddr_port", fPort ) != B_OK )
    {
        return B_ERROR;
    }

    if ( into->AddInt32( "bnaddr_addr", fAddress ) != B_OK )
    {
        return B_ERROR;
    }

    return B_OK;
}


/* Instantiate
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Un-serialize and instantiate from the passed BMessage parameter.
 *
 * Input parameter:
 *     archive      : Archived BMessage object for (de)serialization.
 *
 * Returns:
 *     NULL if a BNetAddress instance can not be initialized, otherwise
 *     a new BNetAddress object instantiated from the BMessage parameter.
 */
BArchivable* BNetAddress::Instantiate( BMessage* archive )
{
    if ( !validate_instantiation( archive, "BNetAddress" ) )
    {
        return NULL;
    }

    BNetAddress* bna = new BNetAddress( archive );
    if ( bna == NULL )
    {
        return NULL;
    }

    if ( bna->InitCheck( ) != B_OK )
    {
        delete bna;
        return NULL;
    }

    return bna;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Set hostname and port network address data.
 *
 * Input parameters:
 *     hostname     : Can be one of three things:
 *                    1. An ASCII-string representation of an IP address.
 *                    2. A canonical hostname.
 *                    3. NULL.  If NULL, then by default the address will be
 *                       set to INADDR_ANY (0.0.0.0).
 *     port         : Duh.
 *
 * Returns:
 *     B_OK/B_ERROR for success/failure.
 */
status_t BNetAddress::SetTo( const char* hostname, unsigned short port )
{
    struct hostent* HostEnt;
    in_addr_t       ia;

    ia = INADDR_ANY;

    // Try like all git-out to set the address from the given hostname.
    if ( hostname != NULL )
    {
        // See if the string is an ASCII-fied IP address.
        ia = inet_addr( hostname );
        if ( ia == INADDR_ANY || ia == ( unsigned long )-1 )
        {
            // See if we can resolve the hostname to an IP address.
            HostEnt = gethostbyname( hostname );
            if ( HostEnt != NULL )
            {
                ia = *( int * )HostEnt->h_addr_list[0];
            }
            else
            {
                return B_ERROR;
            }
        }
    }

    fFamily  = AF_INET;
    fPort    = htons( port );
    fAddress = htonl( ia );

    return B_OK;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Set from passed in socket/address info, our preferred mutator.
 *
 * Input parameter:
 *     sa           : Data specifying the host/client connection.
 *
 * Returns:
 *     B_OK.
 */
status_t BNetAddress::SetTo( const struct sockaddr_in& sa )
{
    fFamily  = sa.sin_family;
    fPort    = htons( sa.sin_port );
    fAddress = htonl( sa.sin_addr.s_addr );

    return B_OK;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Set from passed in address and port.
 *
 * Input parameters:
 *     addr         : IP address in network form.
 *     port         : Optional port number.
 *
 * Returns:
 *     B_OK.
 */
status_t BNetAddress::SetTo( in_addr addr, int port )
{
    fFamily  = AF_INET;
    fPort    = htons( port );
    fAddress = htonl( addr.s_addr );

    return B_OK;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Set from passed in address and port.
 *
 * Input parameters:
 *     addr         : Optional IP address in network form.
 *     port         : Optional port number.
 *
 * Returns:
 *     B_OK.
 */
status_t BNetAddress::SetTo( uint32 addr, int port )
{
    fFamily  = AF_INET;
    fPort    = htons( port );
    fAddress = htonl( addr );

    return B_OK;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Set from passed in hostname and protocol/service information.
 *
 * Input parameters:
 *     hostname     : Can be one of three things:
 *                    1. An ASCII-string representation of an IP address.
 *                    2. A canonical hostname.
 *                    3. NULL.  If NULL, then by default the address will be
 *                       set to INADDR_ANY (0.0.0.0).
 *     protocol     : Datagram type, typically "TCP" or "UDP"
 *     service      : The name of the service, such as http, ftp, et al.  This
 *                    must be one of the official service names listed in
 *                    /etc/services -- but you already knew that because
 *                    you're doing network/sockets programming, RIIIGHT???.
 *
 * Returns:
 *     B_OK/B_ERROR on success/failure.
 *
 * Remarks:
 *     The protocol and service input parameters must be one of the official
 *     types listed in /etc/services.  We use these two parameters to
 *     determine the port number (see getservbyname(3)).  This method will
 *     fail if the aforementioned precondition is not met.
 */
status_t BNetAddress::SetTo( const char* hostname, const char* protocol,
                             const char* service )
{
    struct servent* ServiceEntry;

    ServiceEntry = getservbyname( service, protocol );
    if ( ServiceEntry == NULL )
    {
        return B_ERROR;
    }
    // endservent( ); ???

    return SetTo( hostname, ServiceEntry->s_port );
}


/* clone
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Private copy helper method.
 *
 * Input parameter:
 *     RefParam: Instance to clone.
 *
 * Returns:
 *     B_OK for success, B_NO_INIT if RefParam was not properly constructed.
 */
status_t BNetAddress::clone( const BNetAddress& RefParam )
{
    if ( !RefParam.m_init )
    {
        return B_NO_INIT;
    }

    fFamily  = RefParam.fFamily;
    fPort    = RefParam.fPort;
    fAddress = RefParam.fAddress;

    return B_OK;
}

/* Reserved methods, for future evolution */

void BNetAddress::_ReservedBNetAddressFBCCruft1()
{
}

void BNetAddress::_ReservedBNetAddressFBCCruft2()
{
}

void BNetAddress::_ReservedBNetAddressFBCCruft3()
{
}

void BNetAddress::_ReservedBNetAddressFBCCruft4()
{
}

void BNetAddress::_ReservedBNetAddressFBCCruft5()
{
}

void BNetAddress::_ReservedBNetAddressFBCCruft6()
{
}

/*=------------------------------------------------------------------- End -=*/
