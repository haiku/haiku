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
//
//	File Name:		PortLink.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	A helper class for port-based messaging
//
//------------------------------------------------------------------------------
#include <PortLink.h>
#include <PortMessage.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

//#define PLDEBUG
//#define PLD_DEBUG

//#define CAPACITY_CHECKING

#ifdef PLDEBUG
#include <stdio.h>
#endif

#ifdef PLD_DEBUG
#include <stdio.h>
#endif

/*!
	\class PortLinkData PortLink.cpp
	\brief Internal data storage class
	
	PortLinkData objects serve to hold attached data whilst it is waiting to be Flattened() 
	and then Flushed(). There is no need for this to be used outside the PortLink class. 
*/
class PortLinkData
{
public:
	PortLinkData(void);
	~PortLinkData(void);
	status_t Set(const void *data, size_t size);
	char *buffer;
	size_t buffersize;
};

/*!
	\brief Constructor
	\param port A valid target port_id 
*/
PortLink::PortLink(port_id port)
{
#ifdef PLDEBUG
printf("PortLink(%lu)\n",port);
#endif
	target=port;
	port_info pi;
	port_ok=(get_port_info(target,&pi)==B_OK)?true:false;
	capacity=pi.capacity;
	
	// We start out without any data attached to the port message
	opcode=0;
	bufferlength=0;
	replyport=create_port(30,"PortLink reply port");
	attachlist=new BList(0);
#ifdef PLDEBUG
printf("\tPort valid: %s\n",(port_ok)?"true":"false");
printf("\tReply port: %lu\n",replyport);
#endif
	
	// TODO: initialize all attachments pointers to NULL
}

/*!
	\brief Copy Constructor
	\param port A valid target port_id 

	The copy constructor copies everything except a PortLink's attachments.
*/
PortLink::PortLink(const PortLink &link)
{
#ifdef PLDEBUG
printf("PortLink(PortLink*)\n");
#endif
	target=link.target;
	opcode=link.opcode;
	port_ok=link.port_ok;
	capacity=link.capacity;
	bufferlength=0;
	replyport=create_port(30,"PortLink reply port");
	attachlist=new BList(0);
#ifdef PLDEBUG
printf("\tOpcode: %lu\n",opcode);
printf("\tTarget port: %lu\n",target);
printf("\tCapacity: %lu\n",capacity);
printf("\tPort valid: %s\n",(port_ok)?"true":"false");
printf("\tReply port: %lu\n",replyport);
#endif
	// TODO: initialize all attachments pointers to NULL
}

//! Empties all attachments in addition to deleting the attachment list itself
PortLink::~PortLink(void)
{
#ifdef PLDEBUG
printf("~PortLink()\n");
#endif
	// If, for some odd reason, this is deleted with something attached,
	// free the memory used by the attachments. We do not flush the queue
	// because the port may no longer be valid in cases such as the app
	// is in the process of quitting
	if(attachlist->CountItems()>0)
		MakeEmpty();

	delete attachlist;
}

/*!
	\brief Sets the link's message code
	\param code Message code to use
	
	This code is persistent over sending messages - it will not change unless 
	SetOpCode is called again.
*/
void PortLink::SetOpCode(int32 code)
{
#ifdef PLDEBUG
printf("PortLink::SetOpCode(%c%c%c%c)\n",
(char)((code & 0xFF000000) >>  24),
(char)((code & 0x00FF0000) >>  16),
(char)((code & 0x0000FF00) >>  8),
(char)((code & 0x000000FF)) );
#endif
	// Sets the message code. This does not change once the message is sent.
	// Another call to SetOpCode() is required for such things.
	opcode=code;
}

/*!
	\brief Changes the PortLink's target port
	\param port A valid target port_id
*/
void PortLink::SetPort(port_id port)
{
#ifdef PLDEBUG
printf("PortLink::SetPort(%lu)\n",port);
#endif
	// Sets the target port. While not necessary in most uses, this exists
	// mostly to prevent flexibility problems
	target=port;
	port_info pi;
	port_ok=(get_port_info(target,&pi)==B_OK)?true:false;
	capacity=pi.capacity;
}

/*!
	\brief Returns the PortLink's target port
	\return The PortLink's target port
*/
port_id PortLink::GetPort(void)
{
#ifdef PLDEBUG
printf("PortLink::GetPort() returned %lu\n",target);
#endif
	// Simply returns the port at which the object is pointed.
	return target;
}

status_t PortLink::Flush(bigtime_t timeout)
{
#ifdef PLDEBUG
printf("PortLink::Flush()\n");
#endif
	// Fires a message off to the target, complete with attachments.
	int8 *msgbuffer;
	int32 size;
	status_t write_stat=B_OK;
	
	if(!port_ok)
	{
#ifdef PLDEBUG
printf("\tFlush(): invalid port\n");
#endif
		return B_BAD_VALUE;
	}

	if(attachlist->CountItems()>0)
	{
#ifdef PLDEBUG
printf("\tFlush(): flushing %d attachments\n",num_attachments);
#endif
		FlattenData(&msgbuffer,&size);
		
		// Dump message to port, reset attachments, and clean up
		if(timeout!=B_INFINITE_TIMEOUT)
			write_stat=write_port_etc(target,opcode,msgbuffer,size,B_TIMEOUT, timeout);
		else
			write_stat=write_port(target,opcode,msgbuffer,size);
		MakeEmpty();
	}
	else
	{
#ifdef PLDEBUG
printf("\tFlush(): flushing without attachments\n");
#endif
		if(timeout!=B_INFINITE_TIMEOUT)
			write_stat=write_port_etc(target,opcode,NULL,0,B_TIMEOUT, timeout);
		else
			write_stat=write_port(target,opcode,NULL,0);
	}
	return write_stat;
}


int8* PortLink::FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize, bigtime_t timeout)
{
	// Deprecated call which functions exactly like PortLink(PortLink::ReplyData *data)
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(int32*,status_t*,ssize_t*,bigtime_t)\n");
#endif

	if(!port_ok)
	{
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(): bad port\n");
#endif
		*status=B_BAD_VALUE;
		return NULL;
	}

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&replyport,sizeof(port_id))==B_OK)
	{
		bufferlength+=sizeof(port_id);
	}
	else
	{
		delete pld;
		*status=B_ERROR;
		return NULL;
	}

	// Flatten() inlined to make some necessary changes
	int8 *buffer=new int8[bufferlength];
	int8 *bufferindex=buffer;
	size_t size=0;

	// attach our port_id first
	memcpy(bufferindex, pld->buffer, pld->buffersize);
	bufferindex += pld->buffersize;
	size+=pld->buffersize;

	// attach everything else
	for(int i=0;i<attachlist->CountItems();i++)
	{
		pld=(PortLinkData*)attachlist->ItemAt(i);
		memcpy(bufferindex, pld->buffer, pld->buffersize);
		bufferindex += pld->buffersize;
		size+=pld->buffersize;
	}

	// Flush the thing....FOOSH! :P
	write_port(target,opcode,buffer,size);
	MakeEmpty();
	delete buffer;
	
	// Now we wait for the reply
	buffer=NULL;
	if(timeout==B_INFINITE_TIMEOUT)
	{
		*buffersize=port_buffer_size(replyport);
		if(*buffersize>0)
			buffer=(int8*)new int8[*buffersize];
		read_port(replyport,code, buffer, *buffersize);
	}
	else
	{
		*buffersize=port_buffer_size_etc(replyport,0,timeout);
		if(*buffersize==B_TIMED_OUT)
		{
			*status=*buffersize;
			return NULL;
		}
		if(*buffersize>0)
			buffer=(int8*)new int8[*buffersize];
		read_port(replyport,code, buffer, *buffersize);
	}

	// We got this far, so we apparently have some data
	*status=B_OK;
	return buffer;
}


status_t PortLink::FlushWithReply(PortLink::ReplyData *data,bigtime_t timeout)
{
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(ReplyData*,bigtime_t)\n");
#endif
	// Fires a message to the target and then waits for a reply. The target will
	// receive a message with the first item being the port_id to reply to.
	// NOTE: like Flush(), any attached data must be deleted.

	// Effectively, an Attach() call inlined for changes

	if(!port_ok)
	{
#ifdef PLDEBUG
printf("\tFlushWithReply(): invalid port\n");
#endif
		return B_BAD_VALUE;
	}

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&replyport,sizeof(port_id))==B_OK)
	{
		bufferlength+=sizeof(port_id);
	}
	else
	{
#ifdef PLDEBUG
printf("\tFlushWithReply(): unable to assign reply port to data\n");
#endif
		delete pld;
		return B_ERROR;
	}

	// Flatten() inlined to make some necessary changes
	int8 *buffer=new int8[bufferlength];
	int8 *bufferindex=buffer;
	size_t size=0;

	// attach our port_id first
	memcpy(bufferindex, pld->buffer, pld->buffersize);
	bufferindex += pld->buffersize;
	size+=pld->buffersize;

	// attach everything else
	for(int i=0;i<attachlist->CountItems();i++)
	{
		pld=(PortLinkData*)attachlist->ItemAt(i);
		memcpy(bufferindex, pld->buffer, pld->buffersize);
		bufferindex += pld->buffersize;
		size+=pld->buffersize;
	}

	// Flush the thing....FOOSH! :P
	write_port(target,opcode,buffer,size);
	MakeEmpty();
	delete buffer;
	
	// Now we wait for the reply
	if(timeout==B_INFINITE_TIMEOUT)
	{
		data->buffersize=port_buffer_size(replyport);
		if(data->buffersize>0)
		{
			if(data->buffer)
				delete data->buffer;
			data->buffer=(int8*)new int8[data->buffersize];
		}
		read_port(replyport,&(data->code),data->buffer, data->buffersize);
	}
	else
	{
		data->buffersize=port_buffer_size_etc(replyport,0,timeout);
		if(data->buffersize==B_TIMED_OUT)
			return B_TIMED_OUT;

		if(data->buffersize>0)
		{
			data->buffer=(int8*)new int8[data->buffersize];
		}
		read_port(replyport,&(data->code),data->buffer, data->buffersize);
	}

	// We got this far, so we apparently have some data
	return B_OK;
}

status_t PortLink::FlushWithReply(PortMessage *msg,bigtime_t timeout)
{
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(PortMessage*,bigtime_t)\n");
#endif
	// Fires a message to the target and then waits for a reply. The target will
	// receive a message with the first item being the port_id to reply to.

	// Effectively, an Attach() call inlined for changes
	if(!port_ok || !msg)
	{
#ifdef PLDEBUG
printf("\tFlushWithReply(): invalid port\n");
#endif
		return B_BAD_VALUE;
	}

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&replyport,sizeof(port_id))==B_OK)
	{
		bufferlength+=sizeof(port_id);
	}
	else
	{
#ifdef PLDEBUG
printf("\tFlushWithReply(): unable to assign reply port to data\n");
#endif
		delete pld;
		return B_ERROR;
	}

	// Flatten() inlined to make some necessary changes
	int8 *buffer=new int8[bufferlength];
	int8 *bufferindex=buffer;
	size_t size=0;

	// attach our port_id first
	memcpy(bufferindex, pld->buffer, pld->buffersize);
	bufferindex += pld->buffersize;
	size+=pld->buffersize;

	// attach everything else
	for(int i=0;i<attachlist->CountItems();i++)
	{
		pld=(PortLinkData*)attachlist->ItemAt(i);
		memcpy(bufferindex, pld->buffer, pld->buffersize);
		bufferindex += pld->buffersize;
		size+=pld->buffersize;
	}

	// Flush the thing....FOOSH! :P
	write_port(target,opcode,buffer,size);
	MakeEmpty();
	delete buffer;
	
	// Now we wait for the reply
	ssize_t rbuffersize;
	int8 *rbuffer=NULL;
	int32 rcode;
	
	if(timeout==B_INFINITE_TIMEOUT)
	{
		rbuffersize=port_buffer_size(replyport);
		if(rbuffersize>0)
			rbuffer=(int8*)new int8[rbuffersize];

		read_port(replyport,&rcode,rbuffer,rbuffersize);
	}
	else
	{
		rbuffersize=port_buffer_size_etc(replyport,0,timeout);
		if(rbuffersize==B_TIMED_OUT)
			return B_TIMED_OUT;

		if(rbuffersize>0)
			rbuffer=(int8*)new int8[rbuffersize];

		read_port(replyport,&rcode,rbuffer,rbuffersize);
	}

	// We got this far, so we apparently have some data
	msg->SetCode(rcode);
	msg->SetBuffer(rbuffer,rbuffersize,false);
	
	return B_OK;
}

status_t PortLink::Attach(const void *data, size_t size)
{
#ifdef PLDEBUG
printf("Attach(%p,%ld)\n",data,size);
#endif
	// This is the member called to attach data to a message. Attachments are
	// treated to be in 'Append' mode, tacking on each attached piece of data
	// to the end of the list.
	
	// Prevent parameter problems
	if(size==0)
	{
#ifdef PLDEBUG
printf("\tAttach(): size invalid -> size=0\n");
#endif
		return B_ERROR;
	}
	
#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
	{
#ifdef PLDEBUG
printf("\tAttach(): bufferlength+size > port capacity\n");
#endif
		return B_NO_MEMORY;
	}
#endif

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(data,size)==B_OK)
	{
		attachlist->AddItem(pld);
		bufferlength+=size;
#ifdef PLDEBUG
printf("\tAttach(): successful\n");
printf("\t\tAttach(): attachments now %u\n", num_attachments);
printf("\t\tAttach(): buffer length is %lu\n", bufferlength);
#endif
	}
	else
	{
#ifdef PLDEBUG
printf("\tAttach(): Couldn't assign data to PortLinkData object\n");
#endif
		delete pld;
		return B_ERROR;
	}

	return B_OK;
}

template <class Type> status_t PortLink::Attach(Type data)
{
	int32 size=sizeof(Type);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
	{
		attachlist->AddItem(pld);
		bufferlength+=size;
	}
	else
	{
		delete pld;
		return B_ERROR;
	}
	return B_OK;
}

void PortLink::FlattenData(int8 **buffer,int32 *size)
{
	// This function is where all the magic happens, but it is strictly internal.
	// It iterates through each PortLinkData object and copies it to the main buffer
	// which ends up, ultimately, being written to the PortLink's target port.
	
	// skip if there aree no attachments
	if(bufferlength<1)
	{
#ifdef PLDEBUG
printf("PortLink::FlattenData: bufferlength<1\n");
#endif
		return;
	}

	*buffer=new int8[bufferlength];
	int8 *bufferindex=*buffer;
	PortLinkData *pld;
	*size=0;
	
	int32 count=attachlist->CountItems();
	for(int i=0;i<count;i++)
	{
		pld=(PortLinkData*)attachlist->ItemAt(i);
		memcpy(bufferindex, pld->buffer, pld->buffersize);
		bufferindex += pld->buffersize;
		*size+=pld->buffersize;
	}
}

void PortLink::MakeEmpty(void)
{
#ifdef PLDEBUG
printf("PortLink::MakeEmpty\n");
#endif
	// Nukes all the attachments currently held by the PortLink class
	PortLinkData *pld;
	int32 count=attachlist->CountItems();
	for(int32 i=0; i<count; i++)
	{
		pld=(PortLinkData*)attachlist->ItemAt(i);
		if(pld)
			delete pld;
	}
	attachlist->MakeEmpty();
	bufferlength=0;
}

PortLinkData::PortLinkData(void)
{
	// Initialize object to empty
	buffersize=0;
	buffer=NULL;
}

PortLinkData::~PortLinkData(void)
{
	// Frees the buffer if we actually used the class to store data
	if(buffersize>0 && buffer!=NULL)
		free(buffer);
}

status_t PortLinkData::Set(const void *data, size_t size)
{
#ifdef PLD_DEBUG
printf("PortLinkData::Set(%p,%lu)\n",data,size);
#endif
	// Function copies the passed to the internal buffers for storage
	if(size>0 && buffersize==0 && data!=NULL)
	{
		buffer=(char *)malloc(size);
		if(!buffer)
		{
#ifdef PLD_DEBUG
printf("\tSet(): Couldn't allocate buffer\n");
#endif
			return B_NO_MEMORY;
		}
		memcpy(buffer, data, size);
		buffersize=size;
#ifdef PLD_DEBUG
printf("\tSet(): SUCCESS\n");
#endif
		return B_OK;
	}

#ifdef PLD_DEBUG
if(size==0)
	printf("\tSet(): given an invalid size\n");
if(buffersize>0)
	printf("\tSet(): buffersize is nonzero\n");
if(buffersize>0)
	printf("\tSet(): data is NULL\n");
#endif

	return B_ERROR;
}
