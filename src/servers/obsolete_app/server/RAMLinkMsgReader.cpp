#include "RAMLinkMsgReader.h"
#include <malloc.h>

RAMLinkMsgReader::RAMLinkMsgReader(int8 *buffer)
 : LinkMsgReader(B_ERROR)
{
	SetBuffer(buffer);
}


RAMLinkMsgReader::RAMLinkMsgReader(void)
 : LinkMsgReader(B_ERROR)
{
	fBuffer=NULL;
	fAttachStart=NULL;
	fPosition=NULL;
	fAttachSize=0;
	fCode=B_ERROR;
}


RAMLinkMsgReader::~RAMLinkMsgReader(void)
{
	// Note that this class does NOT take responsibility for the buffer it reads from.
}



void RAMLinkMsgReader::SetBuffer(int8 *buffer)
{
	if(!buffer)
	{
		fBuffer=NULL;
		fAttachStart=NULL;
		fPosition=NULL;
		fAttachSize=0;
		fCode=B_ERROR;
		return;
	}
	
	fBuffer=buffer;
	fPosition=fBuffer+4;
	
	fCode=*((int32*)fBuffer);
	fAttachSize=*( (size_t*) fPosition);
	fPosition+=sizeof(size_t);
	fAttachStart=fPosition;
}


int8 *RAMLinkMsgReader::GetBuffer(void)
{
	return fBuffer;
}


size_t RAMLinkMsgReader::GetBufferSize(void)
{
	return fAttachSize;
}



status_t RAMLinkMsgReader::Read(void *data, ssize_t size)
{
	if(!fBuffer || fAttachSize==0)
		return B_NO_INIT;
	
	if(size<1)
		return B_BAD_VALUE;
	
	if(fPosition+size > fAttachStart+fAttachSize)
	{
		// read past end of buffer
		return B_BAD_VALUE;
	}
	
	memcpy(data, fPosition, size);
	fPosition+=size;
	return B_OK;
}


status_t RAMLinkMsgReader::ReadString(char **string)
{
	status_t err;
	int32 len = 0;

	err = Read<int32>(&len);
	if (err < B_OK)
		return err;

	if (len)
	{
		*string = (char *)malloc(len);
		if (*string == NULL)
		{
			fPosition -= sizeof(int32);
			return B_NO_MEMORY;
		}

		err = Read(*string, len);
		if (err < B_OK)
		{
			free(*string);
			*string = NULL;
			fPosition -= sizeof(int32);
			return err;
		}
		(*string)[len-1] = '\0';
		return B_OK;
	}
	else
	{
		fPosition -= sizeof(int32);
		return B_ERROR;
	}
}

// "Forbidden" functions :P
status_t RAMLinkMsgReader::GetNextMessage(int32 *code, bigtime_t timeout)
{
	debugger("RAMLinkMsgReader::GetNextMessage is not permitted");
	return B_ERROR;
}


void RAMLinkMsgReader::SetPort(port_id port)
{
	debugger("RAMLinkMsgReader::SetPort is not permitted");
}


port_id RAMLinkMsgReader::GetPort(void)
{
	debugger("RAMLinkMsgReader::GetPort is not permitted");
	return B_ERROR;
}


