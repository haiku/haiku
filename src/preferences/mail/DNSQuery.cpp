#include "DNSQuery.h"

#include <errno.h>
#include <stdio.h>

#include <ByteOrder.h>
#include <FindDirectory.h>
#include <NetAddress.h>
#include <NetEndpoint.h>
#include <Path.h> 

// #define DEBUG 1

#undef PRINT
#ifdef DEBUG
#define PRINT(a...) printf(a)
#else
#define PRINT(a...)
#endif


static vint32 gID = 1;


BRawNetBuffer::BRawNetBuffer()
{
	_Init(NULL, 0);
}


BRawNetBuffer::BRawNetBuffer(off_t size)
{
	_Init(NULL, 0);
	fBuffer.SetSize(size);
}


BRawNetBuffer::BRawNetBuffer(const void* buf, size_t size)
{
	_Init(buf, size);
}


status_t
BRawNetBuffer::AppendUint16(uint16 value)
{
	uint16 netVal = B_HOST_TO_BENDIAN_INT16(value);
	ssize_t sizeW = fBuffer.WriteAt(fWritePosition, &netVal, sizeof(uint16));
	if (sizeW == B_NO_MEMORY)
		return B_NO_MEMORY;
	fWritePosition += sizeof(uint16);
	return B_OK;
}


status_t
BRawNetBuffer::AppendString(const char* string)
{
	size_t length = strlen(string) + 1;
	ssize_t sizeW = fBuffer.WriteAt(fWritePosition, string, length);
	if (sizeW == B_NO_MEMORY)
		return B_NO_MEMORY;
	fWritePosition += length;
	return B_OK;
}


status_t
BRawNetBuffer::ReadUint16(uint16& value)
{
	uint16 netVal;
	ssize_t sizeW = fBuffer.ReadAt(fReadPosition, &netVal, sizeof(uint16));
	if (sizeW == 0)
		return B_ERROR;
	value= B_BENDIAN_TO_HOST_INT16(netVal);
	fReadPosition += sizeof(uint16);
	return B_OK;
}


status_t
BRawNetBuffer::ReadUint32(uint32& value)
{
	uint32 netVal;
	ssize_t sizeW = fBuffer.ReadAt(fReadPosition, &netVal, sizeof(uint32));
	if (sizeW == 0)
		return B_ERROR;
	value= B_BENDIAN_TO_HOST_INT32(netVal);
	fReadPosition += sizeof(uint32);
	return B_OK;
}


status_t
BRawNetBuffer::ReadString(BString& string)
{
	string = "";
	ssize_t bytesRead = _ReadStringAt(string, fReadPosition);
	if (bytesRead < 0)
		return B_ERROR;
	fReadPosition += bytesRead;
	return B_OK;
}


status_t
BRawNetBuffer::SkipReading(off_t skip)
{
	if (fReadPosition + skip > fBuffer.BufferLength())
		return B_ERROR;
	fReadPosition += skip;
	return B_OK;
}


void
BRawNetBuffer::_Init(const void* buf, size_t size)
{
	fWritePosition = 0;
	fReadPosition = 0;
	fBuffer.WriteAt(fWritePosition, buf, size);
}


ssize_t
BRawNetBuffer::_ReadStringAt(BString& string, off_t pos)
{
	if (pos >= fBuffer.BufferLength())
		return -1;

	ssize_t bytesRead = 0;
	char* buffer = (char*)fBuffer.Buffer();
	buffer = &buffer[pos];
	// if the string is compressed we have to follow the links to the
	// sub strings
	while (pos < fBuffer.BufferLength() && *buffer != 0) {
		if (uint8(*buffer) == 192) {
			// found a pointer mark
			buffer++;
			bytesRead++;
			off_t subPos = uint8(*buffer);
			_ReadStringAt(string, subPos);
			break;
		}
		string.Append(buffer, 1);
		buffer++;
		bytesRead++;
	}
	bytesRead++;
	return bytesRead;
}


// #pragma mark - DNSTools


status_t
DNSTools::GetDNSServers(BObjectList<BString>* serverList)
{
	// TODO: reading resolv.conf ourselves shouldn't be needed.
	// we should have some function to retrieve the dns list
#define	MATCH(line, name) \
	(!strncmp(line, name, sizeof(name) - 1) && \
	(line[sizeof(name) - 1] == ' ' || \
	 line[sizeof(name) - 1] == '\t'))

	BPath path;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ENTRY_NOT_FOUND;

	path.Append("network/resolv.conf");

	register FILE* fp = fopen(path.Path(), "r");
	if (fp == NULL) {
		fprintf(stderr, "failed to open '%s' to read nameservers: %s\n",
			path.Path(), strerror(errno));
		return B_ENTRY_NOT_FOUND;
	}

	int nserv = 0;
	char buf[1024];
	register char *cp; //, **pp;
//	register int n;
	int MAXNS = 2;

	// read the config file
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		// skip comments
		if (*buf == ';' || *buf == '#')
			continue;

		// read nameservers to query
		if (MATCH(buf, "nameserver") && nserv < MAXNS) {
//			char sbuf[2];
			cp = buf + sizeof("nameserver") - 1;
			while (*cp == ' ' || *cp == '\t')
				cp++;
			cp[strcspn(cp, ";# \t\n")] = '\0';
			if ((*cp != '\0') && (*cp != '\n')) {
				serverList->AddItem(new BString(cp));
				nserv++;
			}
		}
		continue;
	}

	fclose(fp);
	
	return B_OK;
}


BString
DNSTools::ConvertToDNSName(const BString& string)
{
	BString outString = string;
	int32 dot, lastDot, diff;

	dot = string.FindFirst(".");
	if (dot != B_ERROR) {
		outString.Prepend((char*)&dot, 1);
		// because we prepend a char add 1 more
		lastDot = dot + 1;

		while (true) {
			dot = outString.FindFirst(".", lastDot + 1);
			if (dot == B_ERROR)
				break;

			// set a counts to the dot
			diff =  dot - 1 - lastDot;
			outString[lastDot] = (char)diff;
			lastDot = dot;
		}
	} else
		lastDot = 0;

	diff = outString.CountChars() - 1 - lastDot;
	outString[lastDot] = (char)diff;

	return outString;
}


BString
DNSTools::ConvertFromDNSName(const BString& string)
{
	if (string.Length() == 0)
		return string;

	BString outString = string;
	int32 dot = string[0];
	int32 nextDot = dot;
	outString.Remove(0, sizeof(char));
	while (true) {
		if (nextDot >= outString.Length())
			break;
		dot = outString[nextDot];
		if (dot == 0)
			break;
		// set a "."
		outString[nextDot] = '.';
		nextDot+= dot + 1;
	}
	return outString;
}


// #pragma mark - DNSQuery
// see http://tools.ietf.org/html/rfc1035 for more information about DNS


DNSQuery::DNSQuery()
{
}


DNSQuery::~DNSQuery()
{
}


status_t
DNSQuery::ReadDNSServer(in_addr* add)
{
	// list owns the items
	BObjectList<BString> dnsServerList(5, true);
	status_t status = DNSTools::GetDNSServers(&dnsServerList);
	if (status != B_OK)
		return status;
		
	BString* firstDNS = dnsServerList.ItemAt(0);
	if (firstDNS == NULL || inet_aton(firstDNS->String(), add) != 1)
		return B_ERROR;

	PRINT("dns server found: %s \n", firstDNS->String());
	return B_OK;
}


status_t
DNSQuery::GetMXRecords(const BString&  serverName,
	BObjectList<mx_record>* mxList, bigtime_t timeout)
{
	// get the DNS server to ask for the mx record
	in_addr dnsAddress;
	if (ReadDNSServer(&dnsAddress) != B_OK)
		return B_ERROR;

	// create dns query package
	BRawNetBuffer buffer;
	dns_header header;
	_SetMXHeader(&header);
	_AppendQueryHeader(buffer, &header);

	BString serverNameConv = DNSTools::ConvertToDNSName(serverName);
	buffer.AppendString(serverNameConv);
	buffer.AppendUint16(uint16(MX_RECORD));
	buffer.AppendUint16(uint16(1));

	// send the buffer
	PRINT("send buffer\n");
	BNetAddress netAddress(dnsAddress, 53);
	BNetEndpoint netEndpoint(SOCK_DGRAM);
	if (netEndpoint.InitCheck() != B_OK)
		return B_ERROR;

	if (netEndpoint.Connect(netAddress) != B_OK)
		return B_ERROR;
	PRINT("Connected\n");

	int32 bytesSend = netEndpoint.Send(buffer.Data(), buffer.Size());
	if (bytesSend == B_ERROR)
		return B_ERROR;
	PRINT("bytes send %i\n", int(bytesSend));

	// receive buffer
	BRawNetBuffer receiBuffer(512);
	netEndpoint.SetTimeout(timeout);

	int32 bytesRecei = netEndpoint.ReceiveFrom(receiBuffer.Data(), 512,
		netAddress);
	if (bytesRecei == B_ERROR)
		return B_ERROR;
	PRINT("bytes received %i\n", int(bytesRecei));

	dns_header receiHeader;

	_ReadQueryHeader(receiBuffer, &receiHeader);
	PRINT("Package contains :");
	PRINT("%d Questions, ", receiHeader.q_count);
	PRINT("%d Answers, ", receiHeader.ans_count);
	PRINT("%d Authoritative Servers, ", receiHeader.auth_count);
	PRINT("%d Additional records\n", receiHeader.add_count);

	// remove name and Question
	BString dummyS;
	uint16 dummy;
	receiBuffer.ReadString(dummyS);
	receiBuffer.ReadUint16(dummy);
	receiBuffer.ReadUint16(dummy);

	bool mxRecordFound = false;
	for (int i = 0; i < receiHeader.ans_count; i++) {
		resource_record_head rrHead;
		_ReadResourceRecord(receiBuffer, &rrHead);
		if (rrHead.type == MX_RECORD) {
			mx_record* mxRec = new mx_record;
			_ReadMXRecord(receiBuffer, mxRec);
			PRINT("MX record found pri %i, name %s\n",
				mxRec->priority, mxRec->serverName.String());
			// Add mx record to the list
			mxList->AddItem(mxRec);
			mxRecordFound = true;
		} else {
			buffer.SkipReading(rrHead.dataLength);
		}
	}

	if (!mxRecordFound)
		return B_ERROR;

	return B_OK;
}


uint16
DNSQuery::_GetUniqueID()
{
	int32 nextId= atomic_add(&gID, 1);
	// just to be sure
	if (nextId > 65529)
		nextId = 0;
	return nextId;
}


void
DNSQuery::_SetMXHeader(dns_header* header)
{
	header->id = _GetUniqueID();
	header->qr = 0;      //This is a query
	header->opcode = 0;  //This is a standard query
	header->aa = 0;      //Not Authoritative
	header->tc = 0;      //This message is not truncated
	header->rd = 1;      //Recursion Desired
	header->ra = 0;      //Recursion not available! hey we dont have it (lol)
	header->z  = 0;
	header->rcode = 0;
	header->q_count = 1;   //we have only 1 question
	header->ans_count  = 0;
	header->auth_count = 0;
	header->add_count  = 0;
}


void
DNSQuery::_AppendQueryHeader(BRawNetBuffer& buffer, const dns_header* header)
{
	buffer.AppendUint16(header->id);
	uint16 data = 0;
	data |= header->rcode;
	data |= header->z << 4;
	data |= header->ra << 7;
	data |= header->rd << 8;
	data |= header->tc << 9;
	data |= header->aa << 10;
	data |= header->opcode << 11;
	data |= header->qr << 15;
	buffer.AppendUint16(data);
	buffer.AppendUint16(header->q_count);
	buffer.AppendUint16(header->ans_count);
	buffer.AppendUint16(header->auth_count);
	buffer.AppendUint16(header->add_count);
}


void
DNSQuery::_ReadQueryHeader(BRawNetBuffer& buffer, dns_header* header)
{
	buffer.ReadUint16(header->id);
	uint16 data = 0;
	buffer.ReadUint16(data);
	header->rcode = data & 0x0F;
	header->z = (data >> 4) & 0x07;
	header->ra = (data >> 7) & 0x01;
	header->rd = (data >> 8) & 0x01;
	header->tc = (data >> 9) & 0x01;
	header->aa = (data >> 10) & 0x01;
	header->opcode = (data >> 11) & 0x0F;
	header->qr = (data >> 15) & 0x01;
	buffer.ReadUint16(header->q_count);
	buffer.ReadUint16(header->ans_count);
	buffer.ReadUint16(header->auth_count);
	buffer.ReadUint16(header->add_count);
}


void
DNSQuery::_ReadMXRecord(BRawNetBuffer& buffer, mx_record* mxRecord)
{
	buffer.ReadUint16(mxRecord->priority);
	buffer.ReadString(mxRecord->serverName);
	mxRecord->serverName = DNSTools::ConvertFromDNSName(mxRecord->serverName);
}


void
DNSQuery::_ReadResourceRecord(BRawNetBuffer& buffer,
	resource_record_head *rrHead)
{
	buffer.ReadString(rrHead->name);
	buffer.ReadUint16(rrHead->type);
	buffer.ReadUint16(rrHead->dataClass);
	buffer.ReadUint32(rrHead->ttl);
	buffer.ReadUint16(rrHead->dataLength);
}
