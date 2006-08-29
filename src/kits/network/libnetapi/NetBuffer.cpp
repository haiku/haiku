/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott T. Mansfield, thephantom@mac.com
 */

/*!
	Remarks:
	 * This is basically a fancy-schmancy FIFO stack manager.  Thusly, it is
	   considered an error if data is not popped off a particular instance's
	   stack in the order it was pushed on.
	 * We could possibly implement this class as a thin wrapper around the
	   BMessage class, but I'm not sure what kind of payload the latter class
	   brings to the party, so we'll do our own stack management.  We also
	   cannot be sure that BMessage behaves as described above down the road.
	 * Never use hot wax to soothe enraged lobsters.
*/

#include <ByteOrder.h>
#include <Message.h>
#include <NetBuffer.h>

#include <string.h>


/*
 * Parametric ctor initialization list defaults.
 */
#define CTOR_INIT_LIST \
    BArchivable( ), \
    fInit( B_NO_INIT ), \
    fData( NULL ), \
    fDataSize( 0 ), \
    fStackSize( 0 ), \
    fCapacity( 0 )


/*
 * Markers that surround a record in an instance's stack.
 */
#define DATA_START_MARKER   0x01c0ffee
#define DATA_END_MARKER     0xbeefeeee


/* BNetBuffer
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class ctor.
 *
 * Input parameter:
 *     size         : Initial size of buffer (default is zero).
 *
 * Remarks:
 *     This bad boy doubles up as the default ctor.
 */
BNetBuffer::BNetBuffer( size_t size )
           : CTOR_INIT_LIST
{
    fInit = ( resize( size ) == B_OK ) ? B_OK : B_NO_INIT;
}


/* BNetBuffer
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class copy ctor.
 *
 * Input parameter:
 *     refparam     : Instance to copy.
 */
BNetBuffer::BNetBuffer( const BNetBuffer& refparam )
            : CTOR_INIT_LIST
{
    fInit = ( clone( refparam ) == B_OK ) ? B_OK : B_NO_INIT;
}


/* BNetBuffer
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Ctor to instantiate from serialized data (BMessage).
 *
 * Input parameter:
 *     archive      : Serialized object to instantiate from.
 */
BNetBuffer::BNetBuffer( BMessage* archive )
           : CTOR_INIT_LIST
{
    const unsigned char* msgDataPtr;
    ssize_t msgNBytes;

    int32  dataSize;
    int32  stackSize;
    int32  capacity;

    if ( archive->FindInt32( "bnbuff_datasize", &dataSize ) != B_OK )
    {
        return;
    }

    if ( archive->FindInt32( "bnbuff_stacksize", &stackSize ) != B_OK )
    {
        return;
    }

    if ( archive->FindInt32( "bnbuff_capacity", &capacity ) != B_OK )
    {
        return;
    }

    if ( archive->FindData( "bnbuff_stack", B_RAW_TYPE,
                            ( const void** )&msgDataPtr, &msgNBytes ) != B_OK )
    {
        return;
    }

    if ( capacity )
    {
        if ( resize( capacity, true ) != B_OK )
        {
            return;
        }

        memcpy( fData, msgDataPtr, stackSize );
    }

    fDataSize = dataSize;
    fStackSize = stackSize;
    fCapacity = capacity;

    fInit = B_OK;
}


/* ~BNetBuffer
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class dtor.
 */
BNetBuffer::~BNetBuffer( void )
{
    if ( fData != NULL )
    {
        delete fData;
        fData = NULL;
    }

    fDataSize = fStackSize = fCapacity = 0;

    fInit = B_NO_INIT;
}


/* operator=
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class' assignment operator.
 *
 * Input parameter:
 *     refparam     : Instance to assign from.
 */
BNetBuffer& BNetBuffer::operator=( const BNetBuffer& refparam )
{
    fInit = clone( refparam );

    return *this;
}


/* InitCheck
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Determine whether or not this instance is properly initialized.
 *
 * Returns:
 *     B_OK if this instance is initialized, B_ERROR if not.
 */
status_t BNetBuffer::InitCheck( void )
{
    return ( fInit == B_OK ) ? B_OK : B_ERROR;
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
status_t BNetBuffer::Archive( BMessage* into, bool deep ) const
{
    if ( fInit != B_OK )
    {
        return B_NO_INIT;
    }

    if ( into->AddInt32( "bnbuff_datasize", fDataSize ) )
    {
        return B_ERROR;
    }

    if ( into->AddInt32( "bnbuff_stacksize", fStackSize ) )
    {
        return B_ERROR;
    }

    if ( into->AddInt32( "bnbuff_capacity", fCapacity ) )
    {
        return B_ERROR;
    }

    if ( into->AddData( "bnbuff_stack", B_RAW_TYPE, fData, fStackSize ) != B_OK )
    //STM: Should we store the *whole* stack instead?......^^^^^^^^^^fCapacity
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
 *     NULL if a BNetBuffer instance can not be initialized, otherwise
 *     a new BNetBuffer object instantiated from the BMessage parameter.
 */
BArchivable* BNetBuffer::Instantiate( BMessage* archive )
{
    if ( !validate_instantiation( archive, "BNetBuffer" ) )
    {
        return NULL;
    }

    BNetBuffer* bnb = new BNetBuffer( archive );
    if ( bnb == NULL )
    {
        return NULL;
    }

    if ( bnb->InitCheck( ) != B_OK )
    {
        delete bnb;
        return NULL;
    }

    return bnb;
}


/* AppendXXX
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Append some data to this buffer (think PUSHing onto a FIFO stack).
 *
 * Returns:
 *     B_OK/B_ERROR for success/failure.  B_NO_INIT if instance not properly
 *     initialized or stuck in some unknown state during construction.
 *
 * Remarks:
 *   * Where appropriate numeric data such as (u)int16, et al, are converted
 *     to network byte order during storage, no need to do this in advance.
 */
status_t BNetBuffer::AppendInt8( int8 Data )
{
    return dpush( B_INT8_TYPE, sizeof( int8 ), &Data );
}

status_t BNetBuffer::AppendUint8( uint8 Data )
{
    return dpush( B_UINT8_TYPE, sizeof( uint8 ), &Data );
}

status_t BNetBuffer::AppendInt16( int16 Data )
{
    int16 locData = B_HOST_TO_BENDIAN_INT16( Data );
    return dpush( B_INT16_TYPE, sizeof( int16 ), &locData );
}

status_t BNetBuffer::AppendUint16( uint16 Data )
{
    uint16 locData = B_HOST_TO_BENDIAN_INT16( Data );
    return dpush( B_UINT16_TYPE, sizeof( uint16 ), &locData );
}

status_t BNetBuffer::AppendInt32( int32 Data )
{
    int32 locData = B_HOST_TO_BENDIAN_INT32( Data );
    return dpush( B_INT32_TYPE, sizeof( int32 ), &locData );
}

status_t BNetBuffer::AppendUint32( uint32 Data )
{
    uint32 locData = B_HOST_TO_BENDIAN_INT32( Data );
    return dpush( B_UINT32_TYPE, sizeof( uint32 ), &locData );
}

status_t BNetBuffer::AppendInt64( int64 Data )
{
    int64 locData = B_HOST_TO_BENDIAN_INT64( Data );
    return dpush( B_INT64_TYPE, sizeof( int64 ), &locData );
}

status_t BNetBuffer::AppendUint64( uint64 Data )
{
    uint64 locData = B_HOST_TO_BENDIAN_INT64( Data );
    return dpush( B_UINT64_TYPE, sizeof( uint64 ), &locData );
}

status_t BNetBuffer::AppendFloat( float Data )
{
    return dpush( B_FLOAT_TYPE, sizeof( float ), &Data );
}

status_t BNetBuffer::AppendDouble( double Data )
{
    return dpush( B_DOUBLE_TYPE, sizeof( double ), &Data );
}

status_t BNetBuffer::AppendString( const char* Data )
{
    return dpush( B_STRING_TYPE, strlen( Data ), Data );
}

status_t BNetBuffer::AppendData( const void* Data, size_t Length )
{
    return dpush( B_RAW_TYPE, Length, Data );
}

status_t BNetBuffer::AppendMessage( const BMessage& Msg )
{
    status_t result;
    ssize_t msgLen;
    char* msgData;

    msgLen = Msg.FlattenedSize( );
    if ( msgLen == 0 )
    {
        //STM: It's possible, but SHOULD we store an empty BMessage?
        //STM: Is an empty BMessage instance even legit in Be/OBOS?
        return B_ERROR;
    }

    if ( ( msgData = new char[msgLen] ) == NULL )
    {
        return B_ERROR; //STM: B_NO_MEM???
    }

    // Don't get tripped on this, just trying to err on the side of caution.
    result = B_ERROR;

    if ( Msg.Flatten( msgData, msgLen ) == B_OK )
    {
        result = dpush( B_MESSAGE_TYPE, msgLen, msgData );
    }

    delete msgData;
    return result;
}


/* RemoveXXXX
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Remove some data from this buffer, think POPping from a FIFO stack.
 *
 * Returns:
 *     B_OK/B_ERROR for success/failure.  B_NO_INIT if instance not properly
 *     initialized or stuck in some unknown state during construction.
 *
 * Remarks:
 *     Where appropriate numeric data such as (u)int16, et al, are converted
 *     from network byte order.  No need to do this ipso post facto.
 */
status_t BNetBuffer::RemoveInt8( int8& Data )
{
    return dpop( B_INT8_TYPE, sizeof( int8 ), &Data );
}

status_t BNetBuffer::RemoveUint8( uint8& Data )
{
    return dpop( B_UINT8_TYPE, sizeof( uint8 ), &Data );
}

status_t BNetBuffer::RemoveInt16( int16& Data )
{
    // dpop will convert this bad boy from network to host byte order.
    return dpop( B_INT16_TYPE, sizeof( int16 ), &Data );
}

status_t BNetBuffer::RemoveUint16( uint16& Data )
{
    // dpop will convert this beast from network to host byte order.
    return dpop( B_UINT16_TYPE, sizeof( uint16 ), &Data );
}

status_t BNetBuffer::RemoveInt32( int32& Data )
{
    // dpop will convert this thang from network to host byte order.
    return dpop( B_INT32_TYPE, sizeof( int32 ), &Data );
}

status_t BNetBuffer::RemoveUint32( uint32& Data )
{
    // dpop will convert this variable from network to host byte order.
    return dpop( B_UINT32_TYPE, sizeof( uint32 ), &Data );
}

status_t BNetBuffer::RemoveInt64( int64& Data )
{
    // dpop will convert this bag-o-bits from network to host byte order.
    return dpop( B_INT64_TYPE, sizeof( int64 ), &Data );
}

status_t BNetBuffer::RemoveUint64( uint64& Data )
{
    // dpop will convert this construct from network to host byte order.
    return dpop( B_UINT64_TYPE, sizeof( uint64 ), &Data );
}

status_t BNetBuffer::RemoveFloat( float& Data )
{
    return dpop( B_FLOAT_TYPE, sizeof( float ), &Data );
}

status_t BNetBuffer::RemoveDouble( double& Data )
{
    return dpop( B_DOUBLE_TYPE, sizeof( double ), &Data );
}

status_t BNetBuffer::RemoveString( char* Data, size_t Length )
{
    return dpop( B_STRING_TYPE, Length, Data );
}

status_t BNetBuffer::RemoveData( void* Data, size_t Length )
{
    return dpop( B_RAW_TYPE, Length, Data );
}

status_t BNetBuffer::RemoveMessage( BMessage& message )
{
    status_t result;
    int32 v;
    unsigned char* stackPtr = fData;
    char* msgData;

    /* 
     * We have to cheat a little bit here and peek ahead of dpop so we can
     * get at the size parameter.  Perform basic checks along the way so we
     * can make sure that we really get to the size parameter and not some
     * bogus value.  Redundant, but necessary.
     */
    v = *( int32 * )stackPtr;
    if ( v != DATA_START_MARKER )
    {
        return B_ERROR;
    }

    stackPtr += sizeof( int32 );
    v = *( int32 * )stackPtr;
    if ( v != B_MESSAGE_TYPE )
    {
        return B_ERROR;
    }

    // Snarf the data size.
    stackPtr += sizeof( int32 );
    v = *( int32 * )stackPtr;

    if ( ( msgData = new char[v] ) == NULL )
    {
        return B_ERROR; //STM: B_NO_MEM???
    }

    if ( dpop( B_MESSAGE_TYPE, v, msgData ) != B_OK )
    {
        delete msgData;
        return B_ERROR;
    }

    result = message.Unflatten( msgData );
    delete msgData;
    return result;
}


/* Data
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor.
 *
 * Returns:
 *   Pointer to class' data suitable for those functions and methods that
 *   expect some kind of "generic" pointer.
 *
 * Remarks:
 *   * This is WEAK and WRONG, as it allows non-members to directly
 *     manipulate an instance's member data without the usual checks in
 *     place.  At the very least the return value should be const.  Ideally,
 *     this method should not exist. <author steps off soapbox>
 */
unsigned char* BNetBuffer::Data( void ) const
{
    // Have I mentioned that this is a Really Bad Thing to do?
    return fData;
    // In case I missed it: returning a non-const pointer to our member data
    // is not a good thing.  Caveat emptor.
}


/* Size
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor.
 *
 * Returns:
 *     How many bytes of data are contained.  This *does not* include the
 *     overhead of the stack control variables (delineators and size).
 */
size_t BNetBuffer::Size( void ) const
{
    return fDataSize;
}


/* BytesRemaining
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Class accessor ...erm... sort of.
 *
 * Returns:
 *     How much capacity this instance has left to store data.
 *
 * Remarks:
 *     This method is essentially meaningless as the class' storage capacity
 *     will grow (or shrink) as necessary (that's an obfuscated way to say
 *     "dynamically re-allocated" boys and girls).  Do not rely on this
 *     particular method in normal use as it refers to a moving target.
 */
size_t BNetBuffer::BytesRemaining( void ) const
{
    return ( fCapacity - fStackSize );
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
 *     status_t indicating success or point of failure (see resize()).
 *     B_NO_INIT if RefParam not properly initialized.
 */
status_t BNetBuffer::clone( const BNetBuffer& RefParam )
{
    if ( !RefParam.fInit )
    {
        return B_NO_INIT;
    }

    if ( resize( RefParam.fCapacity, true ) != B_OK )
    {
        return B_ERROR;
    }

    fDataSize = RefParam.fDataSize;
    fStackSize = RefParam.fStackSize;
    memcpy( fData, RefParam.fData, fStackSize );

    return B_OK;
}


/* dpop
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Pop some data from our stack.
 *
 * Input parameters:
 *     Type         : Expected data type for this record.
 *     Length       : Expected data size for this record.
 *
 * Output parameter:
 *     Data         : Recepticle for popped data.  You are responsible for
 *                    insuring that this is large enough to hold your
 *                    intended data.  If this parameter is NULL, you'll get
 *                    back a B_ERROR.  If the space allocated that this
 *                    parameter points to is not sufficient to contain the
 *                    intended data, you'll likely over-run something else
 *                    in user-space and piss off a few folks in the process.
 *
 * Returns:
 *     B_OK/B_ERROR on success/failure.
 *     B_NO_INIT if this instance not properly initialized.
 */
status_t BNetBuffer::dpop( int32 Type, int32 Length, void* Data )
{
    if ( fInit != B_OK )
    {
        return B_NO_INIT;
    }

    if ( Data == NULL )
    {
        return B_ERROR;
    }

    unsigned char* stackPtr = fData;
    unsigned char* userDataPtr;
    int32 int32size = sizeof( int32 ); // <-- Performance: called a lot here.
    int32 recordSize = ( ( int32size * 4 ) + Length );
    int32 tmp;

    // Validate the start marker.
    tmp = *( int32 * )stackPtr;
    if ( tmp != (int32) DATA_START_MARKER )
    {
        return B_ERROR;
    }

    // Validate the data type.
    stackPtr += int32size;
    tmp =  *( int32 * )stackPtr;
    if ( tmp != Type )
    {
        return B_ERROR;
    }

    // Validate the data size.
    stackPtr += int32size;
    tmp =  *( int32 * )stackPtr;
    if ( tmp != Length )
    {
        return B_ERROR;
    }

    // Stash a pointer to the contained user data, used when we really "pop."
    stackPtr += int32size;
    userDataPtr = stackPtr;

    // Validate the end marker.
    stackPtr += Length;
    tmp = *( int32 * )stackPtr;
    if ( tmp != (int32) DATA_END_MARKER )
    {
        return B_ERROR;
    }

    // Point to the next entry in the stack.
    stackPtr += int32size;

    // Extract the contained data.
    memcpy( Data, userDataPtr, Length );

    // Convert from network byte order if necessary.
    switch ( Type )
    {
        case B_INT16_TYPE:
        case B_UINT16_TYPE:
            *( uint16 * )Data = B_BENDIAN_TO_HOST_INT16( *( uint16 * )Data );
            break;

        case B_INT32_TYPE:
        case B_UINT32_TYPE:
            *( uint32 * )Data = B_BENDIAN_TO_HOST_INT32( *( uint32 * )Data );
            break;

        case B_INT64_TYPE:
        case B_UINT64_TYPE:
            *( uint64 * )Data = B_BENDIAN_TO_HOST_INT64( *( uint64 * )Data );
            break;

        default:
            break;
    }

    // Condense the stack.
    fStackSize -= recordSize;
    memmove( fData, stackPtr, fStackSize );

    // Zero out the unused area of the stack where we memmove'd from.
    memset( &fData[fStackSize], 0, ( fCapacity - fStackSize ) );

    // Update the extents.
    fCapacity += recordSize;
    fDataSize -= Length;

    return B_OK;
}


/* dpush
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Push some data onto our buffer.
 *
 * Input parameters:
 *     Type         : Data type for this record.
 *     Length       : Data length for this record.
 *     Data         : Pointer to data to store.  NO-OP (B_ERROR) if NULL.
 *
 * Returns:
 *     B_OK/B_ERROR on success/failure.
 *     B_NO_INIT if this instance not properly initialized.
 *
 * Remarks:
 *   * No need to worry about host-to-network byte ordering conversion --
 *     since this is a /private/ helper said conversion, where applicable,
 *     takes place in our caller (see AddIntXX if you haven't already passed
 *     them on your way down to this humble helper).
 */
status_t BNetBuffer::dpush( int32 Type, int32 Length, const void* Data )
{
    if ( fInit != B_OK )
    {
        return B_NO_INIT;
    }

    if ( Data == NULL )
    {
        return B_ERROR;
    }

    int32 int32size = sizeof( int32 ); // <--Performance: called a lot here.
    int32 recordSize = ( ( int32size * 4 ) + Length );

    if ( resize( fCapacity + recordSize ) != B_OK )
    {
        return B_ERROR;
    }

    unsigned char* stackPtr = &fData[fStackSize];
    *( int32 * )stackPtr = DATA_START_MARKER;

    stackPtr += int32size;
    *( int32 * )stackPtr = Type;

    stackPtr += int32size;
    *( int32 * )stackPtr = Length;

    stackPtr += int32size;
    memcpy( stackPtr, Data, Length );

    stackPtr += Length;
    *( int32 * )stackPtr = DATA_END_MARKER;

    // Groovy!  All stored, dot our t's and cross our i's.
    fDataSize += Length;
    fStackSize += recordSize;

    return B_OK;
}


/* resize
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *     Grow the internal data buffer to accommodate the requested size.
 *
 * //STM: What about shrinking the buffer to conserve resources?
 *
 * Input parameter:
 *     NewSize      : New size of data buffer.  Duh.
 *     RegenStack   : If true, do not try to preserve the existing stack.
 *                    Default is false (preserve existing data).
 *
 * Returns:
 *     B_OK: Success.
 *     B_NO_MEMORY: if we cannot allocate memory for the data buffer.
 *
 * Remarks:
 *   * In keeping with the "contract programming" development model: this is
 *     a "safe copy" method in that the class' existing member data will not
 *     be disturbed in the event of failure.  When our caller gets back a
 *     success status (B_OK), said caller is guaranteed that all is well.
 *     Laterally, in the event this method returns failure, the caller is
 *     guaranteed that the class' existing member data is left untouched,
 *     thereby preserving the existing instance's integrity.
 *
 *   * Buffer size is adjusted on a 16-byte granularity to keep from beating
 *     the bleep out of the memory pool (can you say "fragmentation" boys and
 *     girls?).  To tweak the granularity, change the GRANULARITY token at the
 *     beginning of this method.  For best results, GRANULARITY should be a
 *     power of two, thankyouverymuch.
 *
 *   * 'RegenStack' and its intented behavior is a bit of a double-negative.
 *     May have to change this to eliminate said double negative if it proves
 *     to be too confusing.
 */
status_t BNetBuffer::resize( int32 NewSize, bool RegenStack )
{
#define GRANULARITY 16

    unsigned char* newData;
    int32 newCapacity;

    newCapacity = ( ( ( NewSize / GRANULARITY ) + 1 ) * GRANULARITY );

    // Don't waste cycles or thrash the memory pool if we don't need to...
    if ( newCapacity <= fCapacity )
    {
        return B_OK;
    }

/*  STM: If we decide to shrink as well as grow, then replace above with:
    if ( newCapacity == fCapacity )
    {
        return B_OK;
    }
*/

    newData = new unsigned char[newCapacity];
    if ( newData == NULL )
    {
        return B_NO_MEMORY;
    }

    // Paranoid?  Pffffsh.  You bet.  :-)
    memset( newData, 0, newCapacity );

    if ( fData != NULL )
    {
        if ( !RegenStack )
        {
            memcpy( newData, fData, fStackSize );
        }

        delete fData;
    }

    fData = newData;
    fCapacity = newCapacity;

    return B_OK;

#undef GRANULARITY
}


/* Reserved methods, for future evolution */

void BNetBuffer::_ReservedBNetBufferFBCCruft1()
{
}

void BNetBuffer::_ReservedBNetBufferFBCCruft2()
{
}

void BNetBuffer::_ReservedBNetBufferFBCCruft3()
{
}

void BNetBuffer::_ReservedBNetBufferFBCCruft4()
{
}

void BNetBuffer::_ReservedBNetBufferFBCCruft5()
{
}

void BNetBuffer::_ReservedBNetBufferFBCCruft6()
{
}


/*=------------------------------------------------------------------- End -=*/
