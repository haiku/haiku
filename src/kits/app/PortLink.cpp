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

#define PLDEBUG
//#define PLD_DEBUG

//#define CAPACITY_CHECKING

#ifdef PLDEBUG
#include <stdio.h>
#endif

#ifdef PLD_DEBUG
#include <stdio.h>
#endif

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
#ifdef PLDEBUG
printf("PortLink(%lu)\n",port);
#endif
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
#ifdef PLDEBUG
printf("\tPort valid: %s\n",(port_ok)?"true":"false");
printf("\tReply port: %lu\n",replyport);
#endif
	
	// TODO: initialize all attachments pointers to NULL
}

PortLink::PortLink(const PortLink &link)
{
#ifdef PLDEBUG
printf("PortLink(PortLink*)\n");
#endif
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

#ifdef PLDEBUG
printf("\tOpcode: %lu\n",opcode);
printf("\tTarget port: %lu\n",target);
printf("\tCapacity: %lu\n",capacity);
printf("\tPort valid: %s\n",(port_ok)?"true":"false");
printf("\tReply port: %lu\n",replyport);
#endif
	// TODO: initialize all attachments pointers to NULL
}

PortLink::~PortLink(void)
{
#ifdef PLDEBUG
printf("~PortLink()\n");
#endif
	// If, for some odd reason, this is deleted with something attached,
	// free the memory used by the attachments. We do not flush the queue
	// because the port may no longer be valid in cases such as the app
	// is in the process of quitting
	if(num_attachments || bufferlength)
		MakeEmpty();
	
	//TODO: Inline the MakeEmpty call in a way such that it ignores num_attachments
	// and deletes all non-NULL pointers, setting them to NULL afterward.
}

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

port_id PortLink::GetPort(void)
{
#ifdef PLDEBUG
printf("PortLink::GetPort() returned %lu\n",target);
#endif
	// Simply returns the port at which the object is pointed.
	return target;
}

status_t PortLink::Flush(bigtime_t timeout=B_INFINITE_TIMEOUT)
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

	if(num_attachments>0)
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


int8* PortLink::FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize, bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	// Deprecated call which functions exactly like PortLink(PortLink::ReplyData *data)
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(int32*,status_t*,ssize_t*,bigtime_t)\n");
#endif
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
	{
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(): too many attachments\n");
#endif
		*status=B_ERROR;
		return NULL;
	}

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


status_t PortLink::FlushWithReply(PortLink::ReplyData *data,bigtime_t timeout=B_INFINITE_TIMEOUT)
{
#ifdef PLDEBUG
printf("PortLink::FlushWithReply(ReplyData*,bigtime_t)\n");
#endif
	// Fires a message to the target and then waits for a reply. The target will
	// receive a message with the first item being the port_id to reply to.
	// NOTE: like Flush(), any attached data must be deleted.

	// Effectively, an Attach() call inlined for changes

	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
	{
#ifdef PLDEBUG
printf("\tFlushWithReply(): too many attachments\n");
#endif
		return B_ERROR;
	}

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
		read_port(replyport,&(data->code),data->buffer, data->buffersize);
	}
	else
	{
		data->buffersize=port_buffer_size_etc(replyport,0,timeout);
		if(data->buffersize==B_TIMED_OUT)
			return B_TIMED_OUT;

		if(data->buffersize>0)
			data->buffer=(int8*)new int8[data->buffersize];
		read_port(replyport,&(data->code),data->buffer, data->buffersize);
	}

	// We got this far, so we apparently have some data
	return B_OK;
}

status_t PortLink::Attach(void *data, size_t size)
{
#ifdef PLDEBUG
printf("Attach(%p,%ld)\n",data,size);
#endif
	// This is the member called to attach data to a message. Attachments are
	// treated to be in 'Append' mode, tacking on each attached piece of data
	// to the end of the list.
	
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
	{
#ifdef PLDEBUG
printf("\tAttach(): too many attachments\n");
#endif
		return B_ERROR;
	}

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
		num_attachments++;
		attachments[num_attachments-1]=pld;
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
	}

	return B_OK;
}

// These functions were added for a major convenience in passing common types
// Eventually, I'd like to templatize these, but for now, this'll do

status_t PortLink::Attach(int32 data)
{
#ifdef PLDEBUG
printf("Attach(%ld)\n",data);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int32);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(%d)\n",data);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int16);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(%d)\n",data);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(int8);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(%f)\n",data);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(float);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(%s)\n",(data)?"true":"false");
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(bool);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(BRect(%f,%f,%f,%f))\n",data.left,data.top,data.right,data.bottom);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(BRect);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("Attach(BPoint(%f,%f))\n",data.x,data.y);
#endif
	// Prevent parameter problems
	if(num_attachments>=_PORTLINK_MAX_ATTACHMENTS)
		return B_ERROR;

	int32 size=sizeof(BPoint);

#ifdef CAPACITY_CHECKING
	if(bufferlength+size>capacity)
		return B_NO_MEMORY;
#endif
	
	// create a new storage object and stash the data
	PortLinkData *pld=new PortLinkData;
	if(pld->Set(&data,size)==B_OK)
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
#ifdef PLDEBUG
printf("PortLink::MakeEmpty\n");
#endif
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
