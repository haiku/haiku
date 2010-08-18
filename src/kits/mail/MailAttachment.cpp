/*
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 */


/*! Classes which handle mail attachments */


#include <MailAttachment.h>

#include <stdlib.h>
#include <stdio.h>

#include <ByteOrder.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <String.h>

#include <AutoDeleter.h>

#include <mail_encoding.h>
#include <NodeMessage.h>


/*! No attributes or awareness of the file system at large
*/
BSimpleMailAttachment::BSimpleMailAttachment()
	:
	fStatus(B_NO_INIT),
	_data(NULL),
	_raw_data(NULL),
	_we_own_data(false)
{
	Initialize(base64);
}


BSimpleMailAttachment::BSimpleMailAttachment(BPositionIO *data,
	mail_encoding encoding)
	:
	_data(data),
	_raw_data(NULL),
	_we_own_data(false)
{
	fStatus = data == NULL ? B_BAD_VALUE : B_OK;

	Initialize(encoding);
}


BSimpleMailAttachment::BSimpleMailAttachment(const void *data, size_t length,
	mail_encoding encoding)
	:
	_data(new BMemoryIO(data,length)),
	_raw_data(NULL),
	_we_own_data(true)
{
	fStatus = data == NULL ? B_BAD_VALUE : B_OK;

	Initialize(encoding);
}


BSimpleMailAttachment::BSimpleMailAttachment(BFile *file, bool deleteWhenDone)
	:
	_data(NULL),
	_raw_data(NULL),
	_we_own_data(false)
{
	Initialize(base64);
	SetTo(file, deleteWhenDone);
}


BSimpleMailAttachment::BSimpleMailAttachment(entry_ref *ref)
	:
	_data(NULL),
	_raw_data(NULL),
	_we_own_data(false)
{
	Initialize(base64);
	SetTo(ref);
}


BSimpleMailAttachment::~BSimpleMailAttachment()
{
	if (_we_own_data)
		delete _data;
}


void
BSimpleMailAttachment::Initialize(mail_encoding encoding)
{
	SetEncoding(encoding);
	SetHeaderField("Content-Disposition","BMailAttachment");
}


status_t
BSimpleMailAttachment::SetTo(BFile *file, bool deleteFileWhenDone)
{
	char type[B_MIME_TYPE_LENGTH] = "application/octet-stream";

	BNodeInfo nodeInfo(file);
	if (nodeInfo.InitCheck() == B_OK)
		nodeInfo.GetType(type);

	SetHeaderField("Content-Type", type);
	// TODO: No way to get file name (see SetTo(entry_ref *))
	//SetFileName(ref->name);

	if (deleteFileWhenDone)
		SetDecodedDataAndDeleteWhenDone(file);
	else
		SetDecodedData(file);

	return fStatus = B_OK;
}


status_t
BSimpleMailAttachment::SetTo(entry_ref *ref)
{
	BFile *file = new BFile(ref, B_READ_ONLY);
	if ((fStatus = file->InitCheck()) < B_OK) {
		delete file;
		return fStatus;
	}

	if (SetTo(file, true) != B_OK)
		return fStatus;

	SetFileName(ref->name);
	return fStatus = B_OK;
}


status_t
BSimpleMailAttachment::InitCheck()
{
	return fStatus;
}


status_t
BSimpleMailAttachment::FileName(char *text)
{
	BMessage contentType;
	HeaderField("Content-Type", &contentType);

	const char *fileName = contentType.FindString("name");
	if (!fileName)
		fileName = contentType.FindString("filename");
	if (!fileName) {
		contentType.MakeEmpty();
		HeaderField("Content-Disposition", &contentType);
		fileName = contentType.FindString("name");
	}
	if (!fileName)
		fileName = contentType.FindString("filename");
	if (!fileName) {
		contentType.MakeEmpty();
		HeaderField("Content-Location", &contentType);
		fileName = contentType.FindString("unlabeled");
	}
	if (!fileName)
		return B_NAME_NOT_FOUND;

	strncpy(text, fileName, B_FILE_NAME_LENGTH);
	return B_OK;
}


void
BSimpleMailAttachment::SetFileName(const char *name)
{
	BMessage contentType;
	HeaderField("Content-Type", &contentType);

	if (contentType.ReplaceString("name", name) != B_OK)
		contentType.AddString("name", name);

	// Request that the file name header be encoded in UTF-8 if it has weird
	// characters.  If it is just a plain name, the header will appear normal.
	if (contentType.ReplaceInt32(kHeaderCharsetString, B_MAIL_UTF8_CONVERSION)
			!= B_OK)
		contentType.AddInt32(kHeaderCharsetString, B_MAIL_UTF8_CONVERSION);

	SetHeaderField ("Content-Type", &contentType);
}


status_t
BSimpleMailAttachment::GetDecodedData(BPositionIO *data)
{
	ParseNow();

	if (!_data)
		return B_IO_ERROR;
	if (data == NULL)
		return B_BAD_VALUE;

	char buffer[256];
	ssize_t length;
	_data->Seek(0,SEEK_SET);

	while ((length = _data->Read(buffer, sizeof(buffer))) > 0)
		data->Write(buffer, length);

	return B_OK;
}


BPositionIO *
BSimpleMailAttachment::GetDecodedData()
{
	ParseNow();
	return _data;
}


status_t
BSimpleMailAttachment::SetDecodedDataAndDeleteWhenDone(BPositionIO *data)
{
	_raw_data = NULL;

	if (_we_own_data)
		delete _data;

	_data = data;
	_we_own_data = true;

	return B_OK;
}


status_t
BSimpleMailAttachment::SetDecodedData(BPositionIO *data)
{
	_raw_data = NULL;

	if (_we_own_data)
		delete _data;

	_data = data;
	_we_own_data = false;

	return B_OK;
}


status_t
BSimpleMailAttachment::SetDecodedData(const void *data, size_t length)
{
	_raw_data = NULL;

	if (_we_own_data)
		delete _data;

	_data = new BMemoryIO(data,length);
	_we_own_data = true;

	return B_OK;
}


void
BSimpleMailAttachment::SetEncoding(mail_encoding encoding)
{
	_encoding = encoding;

	const char *cte = NULL; //--Content Transfer Encoding
	switch (_encoding) {
		case base64:
			cte = "base64";
			break;
		case seven_bit:
		case no_encoding:
			cte = "7bit";
			break;
		case eight_bit:
			cte = "8bit";
			break;
		case uuencode:
			cte = "uuencode";
			break;
		case quoted_printable:
			cte = "quoted-printable";
			break;
		default:
			cte = "bug-not-implemented";
			break;
	}

	SetHeaderField("Content-Transfer-Encoding", cte);
}


mail_encoding
BSimpleMailAttachment::Encoding()
{
	return _encoding;
}


status_t
BSimpleMailAttachment::SetToRFC822(BPositionIO *data, size_t length,
	bool parseNow)
{
	//---------Massive memory squandering!---ALERT!----------
	if (_we_own_data)
		delete _data;

	off_t position = data->Position();
	BMailComponent::SetToRFC822(data, length, parseNow);

	// this actually happens...
	if (data->Position() - position > length)
		return B_ERROR;

	length -= (data->Position() - position);

	_raw_data = data;
	_raw_length = length;
	_raw_offset = data->Position();

	BString encoding = HeaderField("Content-Transfer-Encoding");
	if (encoding.IFindFirst("base64") >= 0)
		_encoding = base64;
	else if (encoding.IFindFirst("quoted-printable") >= 0)
		_encoding = quoted_printable;
	else if (encoding.IFindFirst("uuencode") >= 0)
		_encoding = uuencode;
	else if (encoding.IFindFirst("7bit") >= 0)
		_encoding = seven_bit;
	else if (encoding.IFindFirst("8bit") >= 0)
		_encoding = eight_bit;
	else
		_encoding = no_encoding;

	if (parseNow)
		ParseNow();

	return B_OK;
}


void
BSimpleMailAttachment::ParseNow()
{
	if (_raw_data == NULL || _raw_length == 0)
		return;

	_raw_data->Seek(_raw_offset, SEEK_SET);

	char *src = (char *)malloc(_raw_length);
	if (src == NULL)
		return;

	size_t size = _raw_length;

	size = _raw_data->Read(src, _raw_length);

	BMallocIO *buffer = new BMallocIO;
	buffer->SetSize(size);
		// 8bit is *always* more efficient than an encoding, so the buffer
		// will *never* be larger than before

	size = decode(_encoding,(char *)(buffer->Buffer()),src,size,0);
	free(src);

	buffer->SetSize(size);

	_data = buffer;
	_we_own_data = true;

	_raw_data = NULL;

	return;
}


status_t
BSimpleMailAttachment::RenderToRFC822(BPositionIO *renderTo)
{
	ParseNow();
	BMailComponent::RenderToRFC822(renderTo);
	//---------Massive memory squandering!---ALERT!----------

	_data->Seek(0, SEEK_END);
	off_t size = _data->Position();
	char *src = (char *)malloc(size);
	if (src == NULL)
		return B_NO_MEMORY;

	MemoryDeleter sourceDeleter(src);

	_data->Seek(0, SEEK_SET);

	ssize_t read = _data->Read(src, size);
	if (read < B_OK)
		return read;

	// The encoded text will never be more than twice as large with any
	// conceivable encoding.  But just in case, there's a function call which
	// will tell us how much space is needed.
	ssize_t destSize = max_encoded_length(_encoding, read);
	if (destSize < B_OK) // Invalid encodings like uuencode rejected here.
		return destSize;
	char *dest = (char *)malloc(destSize);
	if (dest == NULL)
		return B_NO_MEMORY;

	MemoryDeleter destinationDeleter(dest);

	destSize = encode (_encoding, dest, src, read, false /* headerMode */);
	if (destSize < B_OK)
		return destSize;

	if (destSize > 0)
		read = renderTo->Write(dest, destSize);

	return read > 0 ? B_OK : read;
}


//	#pragma mark -


/*!	Supports and sends attributes.
*/
BAttributedMailAttachment::BAttributedMailAttachment()
	:
	fContainer(NULL),
	fStatus(B_NO_INIT),
	_data(NULL),
	_attributes_attach(NULL)
{
}


BAttributedMailAttachment::BAttributedMailAttachment(BFile *file,
	bool deleteWhenDone)
	:
	fContainer(NULL),
	_data(NULL),
	_attributes_attach(NULL)
{
	SetTo(file, deleteWhenDone);
}


BAttributedMailAttachment::BAttributedMailAttachment(entry_ref *ref)
	:
	fContainer(NULL),
	_data(NULL),
	_attributes_attach(NULL)
{
	SetTo(ref);
}


BAttributedMailAttachment::~BAttributedMailAttachment()
{
	// Our SimpleAttachments are deleted by fContainer
	delete fContainer;
}


status_t
BAttributedMailAttachment::Initialize()
{
	// _data & _attributes_attach will be deleted by the container
	delete fContainer;

	fContainer = new BMIMEMultipartMailContainer("++++++BFile++++++");

	_data = new BSimpleMailAttachment();
	fContainer->AddComponent(_data);

	_attributes_attach = new BSimpleMailAttachment();
	_attributes.MakeEmpty();
	_attributes_attach->SetHeaderField("Content-Type",
		"application/x-be_attribute; name=\"BeOS Attributes\"");
	fContainer->AddComponent(_attributes_attach);

	fContainer->SetHeaderField("Content-Type", "multipart/x-bfile");
	fContainer->SetHeaderField("Content-Disposition", "BMailAttachment");

	// also set the header fields of this component, in case someone asks
	SetHeaderField("Content-Type", "multipart/x-bfile");
	SetHeaderField("Content-Disposition", "BMailAttachment");

	return B_OK;
}


status_t
BAttributedMailAttachment::SetTo(BFile *file, bool deleteFileWhenDone)
{
	if (file == NULL)
		return fStatus = B_BAD_VALUE;

	if ((fStatus = Initialize()) < B_OK)
		return fStatus;

	_attributes << *file;

	if ((fStatus = _data->SetTo(file, deleteFileWhenDone)) < B_OK)
		return fStatus;

	// Set boundary

	// Also, we have the make up the boundary out of whole cloth
	// This is likely to give a completely random string
	BString boundary;
	boundary << "BFile--" << (int32(file) ^ time(NULL)) << "-"
		<< ~((int32)file ^ (int32)&fStatus ^ (int32)&_attributes) << "--";
	fContainer->SetBoundary(boundary.String());

	return fStatus = B_OK;
}


status_t
BAttributedMailAttachment::SetTo(entry_ref *ref)
{
	if (ref == NULL)
		return fStatus = B_BAD_VALUE;

	if ((fStatus = Initialize()) < B_OK)
		return fStatus;

	BNode node(ref);
	if ((fStatus = node.InitCheck()) < B_OK)
		return fStatus;

	_attributes << node;

	if ((fStatus = _data->SetTo(ref)) < B_OK)
		return fStatus;

	// Set boundary

	// This is likely to give a completely random string
	BString boundary;
	char buffer[512];
	strcpy(buffer, ref->name);
	for (int32 i = strlen(buffer); i-- > 0;) {
		if (buffer[i] & 0x80)
			buffer[i] = 'x';
		else if (buffer[i] == ' ' || buffer[i] == ':')
			buffer[i] = '_';
	}
	buffer[32] = '\0';
	boundary << "BFile-" << buffer << "--" << ((int32)_data ^ time(NULL))
		<< "-" << ~((int32)_data ^ (int32)&buffer ^ (int32)&_attributes)
		<< "--";
	fContainer->SetBoundary(boundary.String());

	return fStatus = B_OK;
}


status_t
BAttributedMailAttachment::InitCheck()
{
	return fStatus;
}


void
BAttributedMailAttachment::SaveToDisk(BEntry *entry)
{
	BString path = "/tmp/";
	char name[255] = "";
	_data->FileName(name);
	path << name;

	BFile file(path.String(), B_READ_WRITE | B_CREATE_FILE);
	(BNode&)file << _attributes;
	_data->GetDecodedData(&file);
	file.Sync();

	entry->SetTo(path.String());
}


void
BAttributedMailAttachment::SetEncoding(mail_encoding encoding)
{
	_data->SetEncoding(encoding);
	if (_attributes_attach != NULL)
		_attributes_attach->SetEncoding(encoding);
}


mail_encoding
BAttributedMailAttachment::Encoding()
{
	return _data->Encoding();
}


status_t
BAttributedMailAttachment::FileName(char *name)
{
	return _data->FileName(name);
}


void
BAttributedMailAttachment::SetFileName(const char *name)
{
	_data->SetFileName(name);
}


status_t
BAttributedMailAttachment::GetDecodedData(BPositionIO *data)
{
	BNode *node = dynamic_cast<BNode *>(data);
	if (node != NULL)
		*node << _attributes;

	_data->GetDecodedData(data);
	return B_OK;
}


status_t
BAttributedMailAttachment::SetDecodedData(BPositionIO *data)
{
	BNode *node = dynamic_cast<BNode *>(data);
	if (node != NULL)
		_attributes << *node;

	_data->SetDecodedData(data);
	return B_OK;
}


status_t
BAttributedMailAttachment::SetToRFC822(BPositionIO *data, size_t length,
	bool parseNow)
{
	status_t err = Initialize();
	if (err < B_OK)
		return err;

	err = fContainer->SetToRFC822(data, length, parseNow);
	if (err < B_OK)
		return err;

	BMimeType type;
	fContainer->MIMEType(&type);
	if (strcmp(type.Type(), "multipart/x-bfile") != 0)
		return B_BAD_TYPE;

	// get data and attributes
	if ((_data = dynamic_cast<BSimpleMailAttachment *>(
			fContainer->GetComponent(0))) == NULL)
		return B_BAD_VALUE;

	if (parseNow) {
		// Force it to make a copy of the data. Needed for forwarding
		// messages hack.
		_data->GetDecodedData();
	}

	if ((_attributes_attach = dynamic_cast<BSimpleMailAttachment *>(
				fContainer->GetComponent(1))) == NULL
		|| _attributes_attach->GetDecodedData() == NULL)
		return B_OK;

	// Convert the attribute binary attachment into a convenient easy to use
	// BMessage.

	int32 len
		= ((BMallocIO *)(_attributes_attach->GetDecodedData()))->BufferLength();
	char *start = (char *)malloc(len);
	if (start == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(start);

	if (_attributes_attach->GetDecodedData()->ReadAt(0, start, len) < len)
		return B_IO_ERROR;

	int32 index = 0;
	while (index < len) {
		char *name = &start[index];
		index += strlen(name) + 1;

		type_code code;
		memcpy(&code, &start[index], sizeof(type_code));
		code = B_BENDIAN_TO_HOST_INT32(code);
		index += sizeof(type_code);

		int64 buf_length;
		memcpy(&buf_length, &start[index], sizeof(buf_length));
		buf_length = B_BENDIAN_TO_HOST_INT64(buf_length);
		index += sizeof(buf_length);

		swap_data(code, &start[index], buf_length, B_SWAP_BENDIAN_TO_HOST);
		_attributes.AddData(name, code, &start[index], buf_length);
		index += buf_length;
	}

	return B_OK;
}


status_t
BAttributedMailAttachment::RenderToRFC822(BPositionIO *renderTo)
{
	BMallocIO *io = new BMallocIO;

#if defined(HAIKU_TARGET_PLATFORM_DANO)
	const
#endif
	char *name;
	type_code type;
	for (int32 i = 0; _attributes.GetInfo(B_ANY_TYPE, i, &name, &type) == B_OK;
			i++) {
		const void *data;
		ssize_t dataLen;
		_attributes.FindData(name, type, &data, &dataLen);
		io->Write(name, strlen(name) + 1);

		type_code swappedType = B_HOST_TO_BENDIAN_INT32(type);
		io->Write(&swappedType, sizeof(type_code));

		int64 length, swapped;
		length = dataLen;
		swapped = B_HOST_TO_BENDIAN_INT64(length);
		io->Write(&swapped,sizeof(int64));

		void *buffer = malloc(dataLen);
		if (buffer == NULL) {
			delete io;
			return B_NO_MEMORY;
		}
		memcpy(buffer, data, dataLen);
		swap_data(type, buffer, dataLen, B_SWAP_HOST_TO_BENDIAN);
		io->Write(buffer, dataLen);
		free(buffer);
	}
	if (_attributes_attach == NULL)
		_attributes_attach = new BSimpleMailAttachment;

	_attributes_attach->SetDecodedDataAndDeleteWhenDone(io);

	return fContainer->RenderToRFC822(renderTo);
}


status_t
BAttributedMailAttachment::MIMEType(BMimeType *mime)
{
	return _data->MIMEType(mime);
}


// #pragma mark - The reserved function stubs


void BMailAttachment::_ReservedAttachment1() {}
void BMailAttachment::_ReservedAttachment2() {}
void BMailAttachment::_ReservedAttachment3() {}
void BMailAttachment::_ReservedAttachment4() {}

void BSimpleMailAttachment::_ReservedSimple1() {}
void BSimpleMailAttachment::_ReservedSimple2() {}
void BSimpleMailAttachment::_ReservedSimple3() {}

void BAttributedMailAttachment::_ReservedAttributed1() {}
void BAttributedMailAttachment::_ReservedAttributed2() {}
void BAttributedMailAttachment::_ReservedAttributed3() {}
