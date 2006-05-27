//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk
//
//  AllocArea implementation (c) 2002 Marcus Overhagen <marcus@overhagen.de>
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

#define MODULE_NAME	"usb stack: "

Stack::Stack()
{
	dprintf(MODULE_NAME"stack init\n");
	//Init the master lock
	m_master = create_sem( 1 , "usb master lock" );
	set_sem_owner( m_master , B_SYSTEM_TEAM );
	
	//Create the data lock
	m_datalock = create_sem( 1 , "usb data lock" );
	set_sem_owner( m_datalock , B_SYSTEM_TEAM );

	//Set the global "data" variable to this
	data = this;

	//Initialise the memory chunks: create 8, 16 and 32 byte-heaps
	//NOTE: This is probably the most ugly code you will see in the
	//whole stack. Unfortunately this is needed because of the fact
	//that the compiler doesn't like us to apply pointer arithmethic
	//to (void *) pointers. 
	
	// 8-byte heap
	m_areafreecount[0] = 0;
	m_areas[0] = AllocateArea( &m_logical[0] , &m_physical[0] , B_PAGE_SIZE ,
	                        "8-byte chunk area" );
	if ( m_areas[0] < B_OK )
	{
		dprintf(MODULE_NAME"8-byte chunk area failed to initialise\n" );
		return;
	}

	m_8_listhead = (addr_t)m_logical[0];

	for ( int i = 0 ; i < B_PAGE_SIZE/8 ; i++ )
	{
		memory_chunk *chunk = (memory_chunk *)((addr_t)m_logical[0] + 8 * i);
		chunk->physical = (addr_t)m_physical[0] + 8 * i;
		if ( i != B_PAGE_SIZE / 8 - 1 )
			chunk->next_item = (addr_t)m_logical[0] + 8 * ( i + 1 );
		else
			chunk->next_item = NULL;
	}
	
	// 16-byte heap
	m_areafreecount[1] = 0;
	m_areas[1] = AllocateArea( &m_logical[1] , &m_physical[1] , B_PAGE_SIZE ,
	                        "16-byte chunk area" );
	if ( m_areas[1] < B_OK )
	{
		dprintf(MODULE_NAME"16-byte chunk area failed to initialise\n" );
		return;
	}

	m_16_listhead = (addr_t)m_logical[1];

	for ( int i = 0 ; i < B_PAGE_SIZE/16 ; i++ )
	{
		memory_chunk *chunk = (memory_chunk *)((addr_t)m_logical[1] + 16 * i);
		chunk->physical = (addr_t)m_physical[1] + 16 * i;
		if ( i != B_PAGE_SIZE / 16 - 1 )
			chunk->next_item = (addr_t)m_logical[1] + 16 * ( i + 1 );
		else
			chunk->next_item = NULL;
	}

	// 32-byte heap
	m_areafreecount[2] = 0;
	m_areas[2] = AllocateArea( &m_logical[2] , &m_physical[2] , B_PAGE_SIZE ,
	                        "32-byte chunk area" );
	if ( m_areas[2] < B_OK )
	{
		dprintf(MODULE_NAME"32-byte chunk area failed to initialise\n" );
		return;
	}

	m_32_listhead = (addr_t)m_logical[2];

	for ( int i = 0 ; i < B_PAGE_SIZE/32 ; i++ )
	{
		memory_chunk *chunk = (memory_chunk *)((addr_t)m_logical[2] + 32 * i);
		chunk->physical = (addr_t)m_physical[2] + 32 * i;
		if ( i != B_PAGE_SIZE / 32 - 1 )
			chunk->next_item = (addr_t)m_logical[2] + 32 * ( i + 1 );
		else
			chunk->next_item = NULL;
	}

	//Check for host controller modules
	void *list = open_module_list( "busses/usb" );
	char modulename[B_PATH_NAME_LENGTH];
	size_t buffersize = sizeof(modulename);
	dprintf(MODULE_NAME"Looking for host controller modules\n" );
	while( read_next_module_name( list , modulename , &buffersize ) == B_OK )
	{
		dprintf(MODULE_NAME"Found module %s\n" , modulename );
		buffersize = sizeof(modulename);
		host_controller_info *module = 0;
		if ( get_module( modulename , (module_info **)&module ) != B_OK )
			continue;
		if ( module->add_to( *this) != B_OK )
			continue;
		dprintf(MODULE_NAME"module %s successfully loaded\n" , modulename );
	}
	
	if( m_busmodules.Count() == 0 )
		return;
}

Stack::~Stack()
{
	//Release the bus modules
	for( Vector<BusManager *>::Iterator i = m_busmodules.Begin() ; i != m_busmodules.End() ; i++ )
		delete (*i);
	delete_area( m_areas[0] );
	delete_area( m_areas[1] );
	delete_area( m_areas[2] );
}	
	

status_t Stack::InitCheck()
{
	if ( m_busmodules.Count() == 0 )
		return ENODEV;
	return B_OK;
}

void Stack::Lock()
{
	acquire_sem( m_master );
}

void Stack::Unlock()
{
	release_sem( m_master );
}

void Stack::AddBusManager( BusManager *bus )
{
	m_busmodules.PushBack( bus );
}


status_t Stack::AllocateChunk( void **log , void **phy , uint8 size )
{
	Lock();

	addr_t listhead;
	if ( size <= 8 )
		listhead = m_8_listhead;
	else if ( size <= 16 )
		listhead = m_16_listhead;
	else if ( size <= 32 )
		listhead = m_32_listhead;
	else
	{
		dprintf (MODULE_NAME"Chunk size %i to big\n" , size );
		Unlock();
		return B_ERROR;
	}

	if ( listhead == NULL )
	{
		Unlock();
		dprintf(MODULE_NAME"Out of memory on this list\n" );
		return B_ERROR;
	}
	
	dprintf(MODULE_NAME"Stack::Allocate() listhead: %ld\n" , listhead );
	
	memory_chunk *chunk = (memory_chunk *)listhead;
	*log = (void *)listhead;
	*phy = (void *)(chunk->physical);
	if ( chunk->next_item == NULL )
		//TODO: allocate more memory
		listhead = NULL;
	else
		listhead = chunk->next_item;
	
	//Update our listhead pointers
	if ( size <= 8 )
		m_8_listhead = listhead;
	else if ( size <= 16 )
		m_16_listhead = listhead;
	else if ( size <= 32 )
		m_32_listhead = listhead;
	
	Unlock();
	dprintf(MODULE_NAME"allocated a new chunk with size %u\n" , size );
	return B_OK;
}

status_t Stack::FreeChunk( void *log , void *phy , uint8 size )
{	
	Lock();
	addr_t listhead;
	if ( size <= 8 )
		listhead = m_8_listhead;
	else if ( size <= 16 )
		listhead = m_16_listhead;
	else if ( size <= 32 )
		listhead = m_32_listhead;
	else
	{
		dprintf (MODULE_NAME"Chunk size %i invalid\n" , size );
		Unlock();
		return B_ERROR;
	}
	
	memory_chunk *chunk = (memory_chunk *)log;
	
	chunk->next_item = listhead;
	chunk->physical = (addr_t)phy;
	
	if ( size <= 8 )
		m_8_listhead = (addr_t)log;
	else if ( size <= 16 )
		m_16_listhead = (addr_t)log;
	else if ( size <= 32 )
		m_32_listhead = (addr_t)log;
	
	Unlock();
	return B_OK;
}

area_id Stack::AllocateArea( void **log , void **phy , size_t size , const char *name )
{
	physical_entry pe;
	void * logadr;
	area_id areaid;
	status_t rv;
	
	dprintf(MODULE_NAME"allocating %ld bytes for %s\n",size,name);

	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	areaid = create_area(name, &logadr, B_ANY_KERNEL_ADDRESS,size,B_FULL_LOCK | B_CONTIGUOUS, 0 );
	if (areaid < B_OK) {
		dprintf(MODULE_NAME"couldn't allocate area %s\n",name);
		return B_ERROR;
	}
	rv = get_memory_map(logadr,size,&pe,1);
	if (rv < B_OK) {
		delete_area(areaid);
		dprintf(MODULE_NAME"couldn't map %s\n",name);
		return B_ERROR;
	}
	memset(logadr,0,size);
	if (log)
		*log = logadr;
	if (phy)
		*phy = pe.address;
	dprintf(MODULE_NAME"area = %ld, size = %ld, log = 0x%08x, phy = 0x%08x\n",areaid,size,(uint32)logadr,(uint32)(pe.address));
	return areaid;
}
	
Stack *data = 0;
