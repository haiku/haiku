/*
	PortLink.cpp:
		A helper class for port-based messaging

------------------------------------------------------------------------
How it works:
	The class utilizes a fixed-size array of PortLinkData object pointers. When
data is attached, a new PortLinkData object is allocated and a copy of the
passed data is created inside it. When the time comes for the message to be sent,
the data is pieced together into a flattened array and written to the port.
------------------------------------------------------------------------
Data members:

*attachments[]	- fixed-size array of pointers used to hold the attached data
opcode		- message value which is sent along with any data
target		- port to which the message is sent when Flush() is called
replyport	- port used with synchronous messaging - FlushWithReply()
bufferlength - total bytes taken up by attachments
num_attachments	- internal variable which is used to track which "slot"
					will be the next one to receive an attachment object
port_ok - flag to signal whether it's ok to send a message
capacity - contains the storage capacity of the target port.
*/

#include "PortLink.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

//#define PLDEBUG

// Internal data storage class for holding attached data whilst it is waiting
// to be Flattened() and then Flushed(). There is no need for this to be called outside
// the PortLink class. 
class PortLinkData
{
public:
	PortLinkData(void);
	~PortLinkData(void);
	status_t Set(void *data, size_t size);
	char *buffer;
	size_t buffersize;
};

PortLink::PortLink(port_id port)
{
	// For this class to be useful (and to prevent a lot of init problems)
	// we require a port in the constructor
	target=port;
	port_info pi;
	port_ok=(get_port_info(target,&pi)==B_OK)?true:false;
	capacity=pi.capacity;
	
	// We start out without any data attached to the port message
	num_attachments=0;
	opcode=0;
	bufferlength=0;
	replyport=create_port(30,"PortLink reply port");
	
	// TODO: initialize all attachments pointers to NULL
}

PortLink::PortLink(const PortLink &link)
{
	// The copy constructor copies everything except a PortLink's attachments. If there
	// is some reason why someday I might need to change this behavior, I will, but
	// for now there is no real reason I can think of.
	target=link.target;
	opcode=link.opcode;
	port_ok=link.port_ok;
	capacity=link.capacity;
	bufferlength=0;
	num_attachments=0;
	replyport=create_port(30,"PortLink reply port");

	// TODO: initialize all attachments pointers to NULL
}

PortLink::~PortLink(void)
{
	// If, for some odd reason, this is deleted with something attached,
	// free the memory used by the attachments. We do not flush the queue
	// because the port may no longer be valid in cases such as the app
	// is in the process of quitting
	MakeEmpty();
	
	//TODO: Inline the MakeEmpty call in a way such that it ignores num_attachments
	// and deletes all non-NULL pointers, setting them to NULL afterward.
}

void PortLink::SetOpCode(int32 code)
{
	// Sets the message code. This does not change once the message is sent.
	// Another call to SetOpCode() is required for such things.
	opcode=code;
}

void PortLink::SetPort(port_id port)
{
	// Sets the target port. While not necessary in most uses, this exists
	// mostly to prevent flexibility problems
	target=port;
	port_info pi;
	port_ok=(get_port_info(target,&pi)==B_OK)?true:false;
	capacity=pi.capacity;
}

port_id PortLink::GetPort(void)
{
	// Simply returns the port at which the object is pointed.
	return target;
}

status_t PortLink::Flush(bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	// Fires a message off to the target, complete with attachments.
	int8 *msgbuffer;
	int32 size;
	status_t write_stat=B_OK;
	
	if(!port_ok)
		return B_BAD_VALUE;

	if(num_attachments>0)
	{
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
		if(timeout!=B_INFINITE_TIMEOUT)
			write_stat=write_port_etc(target,opcode,NULL,0,B_TIMEOUT, timeout);
		else
			write_stat=write_port(target,opcode,NULL,0);
	}
	return write_stat;
}


int8* PortLink::FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize, bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	// Deprecated call which functions exactly like PortLink(PortLink::ReplyData *data)

	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
	{
		*status=B_ERROR;
		return NULL;
	}

	if(!port_ok)
	{
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
	for(int i=0;i<num_attachments;i++)
	{
		pld=attachments[i];
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


// TODO: write test code
status_t PortLink::FlushWithReply(PortLink::ReplyData *data,bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	// Fires a message to the target and then waits for a reply. The target will
	// receive a message with the first item being the port_id to reply to.
	// NOTE: like Flush(), any attached data must be deleted.

	// Effectively, an Attach() call inlined for changes

	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	if(!port_ok)
		return B_BAD_VALUE;

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&replyport,sizeof(port_id)))
	{
		bufferlength+=sizeof(port_id);
	}
	else
	{
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
	for(int i=0;i<num_attachments;i++)
	{
		pld=attachments[i];
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
			data->buffer=(int8*)new int8[data->buffersize];
		read_port(replyport,&(data->code),&(data->buffer), data->buffersize);
	}
	else
	{
		data->buffersize=port_buffer_size_etc(replyport,0,timeout);
		if(data->buffersize==B_TIMED_OUT)
			return B_TIMED_OUT;

		if(data->buffersize>0)
			data->buffer=(int8*)new int8[data->buffersize];
		read_port(replyport,&(data->code),&(data->buffer), data->buffersize);
	}

	// We got this far, so we apparently have some data
	return B_OK;
}

status_t PortLink::Attach(void *data, size_t size)
{
	// This is the member called to attach data to a message. Attachments are
	// treated to be in 'Append' mode, tacking on each attached piece of data
	// to the end of the list.
	
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	if(size==0)
		return B_ERROR;
	
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;

	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

// These functions were added for a major convenience in passing common types
// Eventually, I'd like to templatize these, but for now, this'll do

status_t PortLink::Attach(int32 data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int32);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(int16 data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int16);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(int8 data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int8);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(float data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(float);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(bool data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(bool);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(BRect data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(BRect);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
	}
	return B_OK;
}

status_t PortLink::Attach(BPoint data)
{
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(BPoint);

	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size))
	{
		num_attachments++;
		attachments[num_attachments-1]=pld;
		bufferlength+=size;
	}
	else
	{
		delete pld;
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
		return;

	*buffer=new int8[bufferlength];
	int8 *bufferindex=*buffer;
	PortLinkData *pld;
	*size=0;
	
	for(int i=0;i<num_attachments;i++)
	{
		pld=attachments[i];
		memcpy(bufferindex, pld->buffer, pld->buffersize);
		bufferindex += pld->buffersize;
		*size+=pld->buffersize;
	}
}

void PortLink::MakeEmpty(void)
{
	// Nukes all the attachments currently held by the PortLink class
	if(num_attachments!=0)
	{
		for(int i=0; i<num_attachments; i++)
		{
			delete attachments[i];
		}
	}
	num_attachments=0;
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

status_t PortLinkData::Set(void *data, size_t size)
{
	// Function copies the passed to the internal buffers for storage
	if(size>0 && buffersize==0 && data!=NULL)
	{
		buffer=(char *)malloc(size);
		if(buffer==NULL)
			return B_NO_MEMORY;
		memcpy(buffer, data, size);
		buffersize=size;
		return B_OK;
	}
	return B_ERROR;
}
