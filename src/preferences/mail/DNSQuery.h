#ifndef DNS_QUERY_H
#define DNS_QUERY_H


#include <DataIO.h>
#include <NetBuffer.h>
#include <ObjectList.h>
#include <String.h>


#include <arpa/inet.h>

#define MX_RECORD		15

struct mx_record {
	uint16	priority;
	BString	serverName;
};


class BRawNetBuffer {
public:
						BRawNetBuffer();
						BRawNetBuffer(off_t size);
						BRawNetBuffer(const void* buf, size_t size);

		// functions like in BNetBuffer but no type and size info is writen.
		// functions return B_NO_MEMORY or B_OK
		status_t		AppendUint16(uint16 value);
		status_t		AppendString(const char* string);

		status_t		ReadUint16(uint16& value);
		status_t		ReadUint32(uint32& value);
		status_t		ReadString(BString& string);

		status_t		SkipReading(off_t off);
		
		void			*Data(void) const { return (void*)fBuffer.Buffer(); }
		size_t			Size(void) const { return fBuffer.BufferLength(); }
		size_t			SetSize(off_t size) { return fBuffer.SetSize(size); }
		void			SetWritePosition(off_t pos) { fWritePosition = pos; }

private:
		void			_Init(const void* buf, size_t size);
		ssize_t			_ReadStringAt(BString& string, off_t pos);

		off_t 			fWritePosition;
		off_t 			fReadPosition;
		BMallocIO		fBuffer;
};


class DNSTools {
public:
		static status_t		GetDNSServers(BObjectList<BString>* serverList);
		static BString	ConvertToDNSName(const BString& string);
		static BString	ConvertFromDNSName(const BString& string);
};

// see also http://prasshhant.blogspot.com/2007/03/dns-query.html
struct dns_header {
	dns_header()
	{
		q_count = 0;
		ans_count  = 0;
		auth_count = 0;
		add_count  = 0;
	}

	uint16 id;						// A 16 bit identifier
	
	unsigned	char qr     :1;		// query (0), or a response (1)
	unsigned	char opcode :4;	    // A four bit field
	unsigned	char aa     :1;		// Authoritative Answer
	unsigned	char tc     :1;		// Truncated Message
	unsigned	char rd     :1;		// Recursion Desired	
	unsigned	char ra     :1;		// Recursion Available
	unsigned	char z      :3;		// Reserved for future use
	unsigned	char rcode  :4;	    // Response code

	uint16		q_count;			// number of question entries
	uint16		ans_count;			// number of answer entries
	uint16		auth_count;			// number of authority entries
	uint16		add_count;			// number of resource entries
};

// resource record without resource data 
struct resource_record_head {
	BString	name;
	uint16	type;
	uint16	dataClass;
	uint32	ttl;
	uint16	dataLength;
};


class DNSQuery {
public:
						DNSQuery();
						~DNSQuery();
		status_t		ReadDNSServer(in_addr* add);
		status_t		GetMXRecords(const BString& serverName,
							BObjectList<mx_record>* mxList,
							bigtime_t timeout = 500000);
				  
private:
		uint16			_GetUniqueID();
		void			_SetMXHeader(dns_header* header);
		void			_AppendQueryHeader(BRawNetBuffer& buffer,
							const dns_header* header);
		void			_ReadQueryHeader(BRawNetBuffer& buffer,
							dns_header* header);
		void			_ReadMXRecord(BRawNetBuffer& buffer,
							mx_record* mxRecord);

		void			_ReadResourceRecord(BRawNetBuffer& buffer,
							resource_record_head* rrHead);
};


#endif // DNS_QUERY_H
