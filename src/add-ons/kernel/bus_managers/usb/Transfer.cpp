//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk
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

#include "usb_p.h"

Transfer::Transfer( Pipe *pipe , bool synchronous )
{
	m_pipe = pipe;
	m_buffer = 0;
	m_bufferlength = 0;
	m_actual_length = 0;
	m_timeout = 0;
	m_status = 0;
	m_sem = B_ERROR;
	m_hcpriv = 0;
	
	if ( synchronous == true )
	{
		m_sem = create_sem( 0 , "USB Transfer" );
		set_sem_owner ( m_sem , B_SYSTEM_TEAM );
	}
}

Transfer::~Transfer()
{
	if ( m_sem > B_OK )
		delete_sem( m_sem );
}

void Transfer::SetRequestData( usb_request_data *data )
{
	m_request = data;
}

void Transfer::SetBuffer( uint8 *buffer )
{
	m_buffer = buffer;
}

void Transfer::SetBufferLength( size_t length )
{
	m_bufferlength = length;
}

void Transfer::SetActualLength( size_t *actual_length )
{
	m_actual_length = actual_length;
};

void Transfer::SetCallbackFunction( usb_callback_func callback )
{
	m_callback = callback;
}

void Transfer::SetHostPrivate( hostcontroller_priv *priv )
{
	m_hcpriv = priv;
}

void Transfer::WaitForFinish()
{
	if ( m_sem > B_OK )
		acquire_sem( m_sem );
}	

void Transfer::TransferDone()
{
	if ( m_sem > B_OK )
		release_sem( m_sem );
}

void Transfer::Finish()
{
	//If we are synchronous, release a sem
	if ( m_sem > B_OK )
		release_sem( m_sem );
}
