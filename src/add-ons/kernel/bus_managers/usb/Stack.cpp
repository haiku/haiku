//------------------------------------------------------------------------------
//	Copyright (c) 2003, Niels S. Reedijk
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

#include <module.h>
#include <util/kernel_cpp.h>

#include "usb_p.h"

Stack::Stack()
{
	//Init the master lock
	m_master = create_sem( 0 , "usb master lock" );
	set_sem_owner( m_master , B_SYSTEM_TEAM );
	
	//Check for host controller modules
	void *list = open_module_list( "busses/usb" );
	char modulename[B_PATH_NAME_LENGTH];
	size_t buffersize = sizeof(modulename);
	dprintf( "USB: Looking for host controller modules\n" );
	while( read_next_module_name( list , modulename , &buffersize ) == B_OK )
	{
		dprintf( "USB: Found module %s\n" , modulename );
		module_info *module = 0;
		if ( get_module( modulename , &module ) != B_OK )
			continue;
		m_busmodules.Insert( module , 0 );
		dprintf( "USB: module %s successfully loaded\n" , modulename );
	}
	
	if( m_busmodules.Count() == 0 )
		return;
}

Stack::~Stack()
{
	//Release the bus modules
	for( Vector<module_info *>::Iterator i = m_busmodules.Begin() ; i != m_busmodules.End() ; i++ )
		put_module( (*i)->name );
}	
	

status_t Stack::InitCheck()
{
	if ( m_busmodules.Count() == 0 )
		return ENODEV;
	return B_OK;
}

Stack *data = 0;
