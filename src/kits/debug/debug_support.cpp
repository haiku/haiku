/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <string.h>

#include <AutoDeleter.h>
#include <debug_support.h>

#include "arch_debug_support.h"
#include "Image.h"
#include "SymbolLookup.h"


using std::nothrow;


struct debug_symbol_lookup_context {
	SymbolLookup*	lookup;
};

struct debug_symbol_iterator : BPrivate::Debug::SymbolIterator {
	bool	ownsImage;

	debug_symbol_iterator()
		:
		ownsImage(false)
	{
	}

	~debug_symbol_iterator()
	{
		if (ownsImage)
			delete image;
	}
};


// init_debug_context
status_t
init_debug_context(debug_context *context, team_id team, port_id nubPort)
{
	if (!context || team < 0 || nubPort < 0)
		return B_BAD_VALUE;

	context->team = team;
	context->nub_port = nubPort;

	// create the reply port
	context->reply_port = create_port(1, "debug reply port");
	if (context->reply_port < 0)
		return context->reply_port;

	return B_OK;
}

// destroy_debug_context
void
destroy_debug_context(debug_context *context)
{
	if (context) {
		if (context->reply_port >= 0)
			delete_port(context->reply_port);

		context->team = -1;
		context->nub_port = -1;
		context->reply_port = -1;
	}
}

// send_debug_message
status_t
send_debug_message(debug_context *context, int32 messageCode,
	const void *message, int32 messageSize, void *reply, int32 replySize)
{
	if (!context)
		return B_BAD_VALUE;

	// send message
	while (true) {
		status_t result = write_port(context->nub_port, messageCode, message,
			messageSize);
		if (result == B_OK)
			break;
		if (result != B_INTERRUPTED)
			return result;
	}

	if (!reply)
		return B_OK;

	// read reply
	while (true) {
		int32 code;
		ssize_t bytesRead = read_port(context->reply_port, &code, reply,
			replySize);
		if (bytesRead > 0)
			return B_OK;
		if (bytesRead != B_INTERRUPTED)
			return bytesRead;
	}
}

// debug_read_memory_partial
ssize_t
debug_read_memory_partial(debug_context *context, const void *address,
	void *buffer, size_t size)
{
	if (!context)
		return B_BAD_VALUE;

	if (size == 0)
		return 0;
	if (size > B_MAX_READ_WRITE_MEMORY_SIZE)
		size = B_MAX_READ_WRITE_MEMORY_SIZE;

	// prepare the message
	debug_nub_read_memory message;
	message.reply_port = context->reply_port;
	message.address = (void*)address;
	message.size = size;

	// send the message
	debug_nub_read_memory_reply reply;
	status_t error = send_debug_message(context, B_DEBUG_MESSAGE_READ_MEMORY,
		&message, sizeof(message), &reply, sizeof(reply));

	if (error != B_OK)
		return error;
	if (reply.error != B_OK)
		return reply.error;

	// copy the read data
	memcpy(buffer, reply.data, reply.size);
	return reply.size;
}

// debug_read_memory
ssize_t
debug_read_memory(debug_context *context, const void *_address, void *_buffer,
	size_t size)
{
	const char *address = (const char *)_address;
	char *buffer = (char*)_buffer;

	// check parameters
	if (!context || !address || !buffer)
		return B_BAD_VALUE;
	if (size == 0)
		return 0;

	// read as long as we can read data
	ssize_t sumRead = 0;
	while (size > 0) {
		ssize_t bytesRead = debug_read_memory_partial(context, address, buffer,
			size);
		if (bytesRead < 0) {
			if (sumRead > 0)
				return sumRead;
			return bytesRead;
		}

		address += bytesRead;
		buffer += bytesRead;
		sumRead += bytesRead;
		size -= bytesRead;
	}

	return sumRead;
}

// debug_read_string
ssize_t
debug_read_string(debug_context *context, const void *_address, char *buffer,
	size_t size)
{
	const char *address = (const char *)_address;

	// check parameters
	if (!context || !address || !buffer || size == 0)
		return B_BAD_VALUE;

	// read as long as we can read data
	ssize_t sumRead = 0;
	while (size > 0) {
		ssize_t bytesRead = debug_read_memory_partial(context, address, buffer,
			size);
		if (bytesRead < 0) {
			// always null-terminate what we have (even, if it is an empty
			// string) and be done
			*buffer = '\0';
			return (sumRead > 0 ? sumRead : bytesRead);
		}

		int chunkSize = strnlen(buffer, bytesRead);
		if (chunkSize < bytesRead) {
			// we found a terminating null
			sumRead += chunkSize;
			return sumRead;
		}

		address += bytesRead;
		buffer += bytesRead;
		sumRead += bytesRead;
		size -= bytesRead;
	}

	// We filled the complete buffer without encountering a terminating null
	// replace the last char. But nevertheless return the full size to indicate
	// that the buffer was too small.
	buffer[-1] = '\0';

	return sumRead;
}

// debug_write_memory_partial
ssize_t
debug_write_memory_partial(debug_context *context, const void *address,
	void *buffer, size_t size)
{
	if (!context)
		return B_BAD_VALUE;

	if (size == 0)
		return 0;
	if (size > B_MAX_READ_WRITE_MEMORY_SIZE)
		size = B_MAX_READ_WRITE_MEMORY_SIZE;

	// prepare the message
	debug_nub_write_memory message;
	message.reply_port = context->reply_port;
	message.address = (void*)address;
	message.size = size;
	memcpy(message.data, buffer, size);

	// send the message
	debug_nub_write_memory_reply reply;
	status_t error = send_debug_message(context, B_DEBUG_MESSAGE_WRITE_MEMORY,
		&message, sizeof(message), &reply, sizeof(reply));

	if (error != B_OK)
		return error;
	if (reply.error != B_OK)
		return reply.error;

	return reply.size;
}

// debug_write_memory
ssize_t
debug_write_memory(debug_context *context, const void *_address, void *_buffer,
	size_t size)
{
	const char *address = (const char *)_address;
	char *buffer = (char*)_buffer;

	// check parameters
	if (!context || !address || !buffer)
		return B_BAD_VALUE;
	if (size == 0)
		return 0;

	ssize_t sumWritten = 0;
	while (size > 0) {
		ssize_t bytesWritten = debug_write_memory_partial(context, address, buffer,
			size);
		if (bytesWritten < 0) {
			if (sumWritten > 0)
				return sumWritten;
			return bytesWritten;
		}

		address += bytesWritten;
		buffer += bytesWritten;
		sumWritten += bytesWritten;
		size -= bytesWritten;
	}

	return sumWritten;
}

// debug_get_cpu_state
status_t
debug_get_cpu_state(debug_context *context, thread_id thread,
	debug_debugger_message *messageCode, debug_cpu_state *cpuState)
{
	if (!context || !cpuState)
		return B_BAD_VALUE;

	// prepare message
	debug_nub_get_cpu_state message;
	message.reply_port = context->reply_port;
	message.thread = thread;

	// send message
	debug_nub_get_cpu_state_reply reply;
	status_t error = send_debug_message(context, B_DEBUG_MESSAGE_GET_CPU_STATE,
		&message, sizeof(message), &reply, sizeof(reply));
	if (error == B_OK)
		error = reply.error;

	// get state
	if (error == B_OK) {
		*cpuState = reply.cpu_state;
		if (messageCode)
			*messageCode = reply.message;
	}

	return error;
}


// #pragma mark -

// debug_get_instruction_pointer
status_t
debug_get_instruction_pointer(debug_context *context, thread_id thread,
	void **ip, void **stackFrameAddress)
{
	if (!context || !ip || !stackFrameAddress)
		return B_BAD_VALUE;

	return arch_debug_get_instruction_pointer(context, thread, ip,
		stackFrameAddress);
}

// debug_get_stack_frame
status_t
debug_get_stack_frame(debug_context *context, void *stackFrameAddress,
	debug_stack_frame_info *stackFrameInfo)
{
	if (!context || !stackFrameAddress || !stackFrameInfo)
		return B_BAD_VALUE;

	return arch_debug_get_stack_frame(context, stackFrameAddress,
		stackFrameInfo);
}


// #pragma mark -

// debug_create_symbol_lookup_context
status_t
debug_create_symbol_lookup_context(team_id team, image_id image,
	debug_symbol_lookup_context **_lookupContext)
{
	if (team < 0 || !_lookupContext)
		return B_BAD_VALUE;

	// create the lookup context
	debug_symbol_lookup_context *lookupContext
		= new(std::nothrow) debug_symbol_lookup_context;
	if (lookupContext == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<debug_symbol_lookup_context> contextDeleter(lookupContext);

	// create and init symbol lookup
	SymbolLookup *lookup = new(std::nothrow) SymbolLookup(team, image);
	if (lookup == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<SymbolLookup> lookupDeleter(lookup);

	try {
		status_t error = lookup->Init();
		if (error != B_OK)
			return error;
	} catch (BPrivate::Debug::Exception& exception) {
		return exception.Error();
	}

	// everything went fine: return the result
	lookupContext->lookup = lookup;
	*_lookupContext = lookupContext;
	contextDeleter.Detach();
	lookupDeleter.Detach();

	return B_OK;
}

// debug_delete_symbol_lookup_context
void
debug_delete_symbol_lookup_context(debug_symbol_lookup_context *lookupContext)
{
	if (lookupContext) {
		delete lookupContext->lookup;
		delete lookupContext;
	}
}


// debug_get_symbol
status_t
debug_get_symbol(debug_symbol_lookup_context* lookupContext, image_id image,
	const char* name, int32 symbolType, void** _symbolLocation,
	size_t* _symbolSize, int32* _symbolType)
{
	if (!lookupContext || !lookupContext->lookup)
		return B_BAD_VALUE;
	SymbolLookup* lookup = lookupContext->lookup;

	return lookup->GetSymbol(image, name, symbolType, _symbolLocation,
		_symbolSize, _symbolType);
}


// debug_lookup_symbol_address
status_t
debug_lookup_symbol_address(debug_symbol_lookup_context *lookupContext,
	const void *address, void **baseAddress, char *symbolName,
	int32 symbolNameSize, char *imageName, int32 imageNameSize,
	bool *exactMatch)
{
	if (!lookupContext || !lookupContext->lookup)
		return B_BAD_VALUE;
	SymbolLookup *lookup = lookupContext->lookup;

	// find the symbol
	addr_t _baseAddress;
	const char *_symbolName;
	size_t _symbolNameLen;
	const char *_imageName;
	try {
		status_t error = lookup->LookupSymbolAddress((addr_t)address,
			&_baseAddress, &_symbolName, &_symbolNameLen, &_imageName,
			exactMatch);
		if (error != B_OK)
			return error;
	} catch (BPrivate::Debug::Exception& exception) {
		return exception.Error();
	}

	// translate/copy the results
	if (baseAddress)
		*baseAddress = (void*)_baseAddress;

	if (symbolName && symbolNameSize > 0) {
		if (_symbolName && _symbolNameLen > 0) {
			strlcpy(symbolName, _symbolName,
				min_c((size_t)symbolNameSize, _symbolNameLen + 1));
		} else
			symbolName[0] = '\0';
	}

	if (imageName) {
		if (imageNameSize > B_PATH_NAME_LENGTH)
			imageNameSize = B_PATH_NAME_LENGTH;
		strlcpy(imageName, _imageName, imageNameSize);
	}

	return B_OK;
}


status_t
debug_create_image_symbol_iterator(debug_symbol_lookup_context* lookupContext,
	image_id imageID, debug_symbol_iterator** _iterator)
{
	if (!lookupContext || !lookupContext->lookup)
		return B_BAD_VALUE;
	SymbolLookup *lookup = lookupContext->lookup;

	debug_symbol_iterator* iterator = new(std::nothrow) debug_symbol_iterator;
	if (iterator == NULL)
		return B_NO_MEMORY;

	status_t error;
	try {
		error = lookup->InitSymbolIterator(imageID, *iterator);
	} catch (BPrivate::Debug::Exception& exception) {
		error = exception.Error();
	}

	// Work-around for a runtime loader problem. A freshly fork()ed child does
	// still have image_t structures with the parent's image ID's, so we
	// wouldn't find the image in this case.
	if (error != B_OK) {
		// Get the image info and re-try looking with the text base address.
		// Note, that we can't easily check whether the image is part of the
		// target team at all (there's no image_info::team, we'd have to
		// iterate through all images).
		image_info imageInfo;
		error = get_image_info(imageID, &imageInfo);
		if (error == B_OK) {
			try {
				error = lookup->InitSymbolIteratorByAddress(
					(addr_t)imageInfo.text, *iterator);
			} catch (BPrivate::Debug::Exception& exception) {
				error = exception.Error();
			}
		}
	}

	if (error != B_OK) {
		delete iterator;
		return error;
	}

	*_iterator = iterator;
	return B_OK;
}


status_t
debug_create_file_symbol_iterator(const char* path,
	debug_symbol_iterator** _iterator)
{
	if (path == NULL)
		return B_BAD_VALUE;

	// create the iterator
	debug_symbol_iterator* iterator = new(std::nothrow) debug_symbol_iterator;
	if (iterator == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<debug_symbol_iterator> iteratorDeleter(iterator);

	// create the image file
	ImageFile* imageFile = new(std::nothrow) ImageFile;
	if (imageFile == NULL)
		return B_NO_MEMORY;

	// init the iterator
	iterator->image = imageFile;
	iterator->ownsImage = true;
	iterator->currentIndex = -1;

	// init the image file
	status_t error = imageFile->Init(path);
	if (error != B_OK)
		return error;

	iteratorDeleter.Detach();
	*_iterator = iterator;

	return B_OK;
}


void
debug_delete_symbol_iterator(debug_symbol_iterator* iterator)
{
	delete iterator;
}


// debug_next_image_symbol
status_t
debug_next_image_symbol(debug_symbol_iterator* iterator, char* nameBuffer,
	size_t nameBufferLength, int32* _symbolType, void** _symbolLocation,
	size_t* _symbolSize)
{
	if (iterator == NULL || iterator->image == NULL)
		return B_BAD_VALUE;

	const char* symbolName;
	size_t symbolNameLen;
	addr_t symbolLocation;

	try {
		status_t error = iterator->image->NextSymbol(iterator->currentIndex,
			&symbolName, &symbolNameLen, &symbolLocation, _symbolSize,
			_symbolType);
		if (error != B_OK)
			return error;
	} catch (BPrivate::Debug::Exception& exception) {
		return exception.Error();
	}

	*_symbolLocation = (void*)symbolLocation;

	if (symbolName != NULL && symbolNameLen > 0) {
		strlcpy(nameBuffer, symbolName,
			min_c(nameBufferLength, symbolNameLen + 1));
	} else
		nameBuffer[0] = '\0';

	return B_OK;
}


status_t
debug_get_symbol_iterator_image_info(debug_symbol_iterator* iterator,
	image_info* info)
{
	if (iterator == NULL || iterator->image == NULL || info == NULL)
		return B_BAD_VALUE;

	*info = iterator->image->Info();
	return B_OK;
}
