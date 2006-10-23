// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __IppContent_H
#define __IppContent_H

#include <iostream>
#include <list>
#include <string>

#if (!__MWERKS__)
using namespace std;
#else 
#define std
#endif

enum IPP_OPERATION_ID {

	/* reserved, not used: 0x0000 */
	/* reserved, not used: 0x0001 */

	IPP_PRINT_JOB				= 0x0002,	// printer operation
	IPP_PRINT_URI				= 0x0003,	// printer operation
	IPP_VALIDATE_JOB			= 0x0004,	// printer operation
	IPP_CREATE_JOB				= 0x0005,	// printer operation

	IPP_SEND_DOCUMENT			= 0x0006,	// job operation
	IPP_SEND_URI				= 0x0007,	// job operation
	IPP_CANCEL_JOB				= 0x0008,	// job operation
	IPP_GET_JOB_ATTRIBUTES		= 0x0009,	// job operation

	IPP_GET_JOBS				= 0x000A,	// printer operation
	IPP_GET_PRINTER_ATTRIBUTES	= 0x000B	// printer operation

	/* reserved for future operations: 0x000C-0x3FFF */
	/* reserved for private extensions: 0x4000-0x8FFF */
};

enum IPP_STATUS_CODE {

	IPP_SUCCESSFUL_OK_S									= 0x0000,	// successful
	IPP_SUCCESSFUL_OK									= 0x0000,	// successful
	IPP_SUCCESSFUL_OK_IGNORED_OR_SUBSTITUTED_ATTRIBUTES	= 0x0001,	// successful
	IPP_SUCCESSFUL_OK_CONFLICTING_ATTRIBUTES			= 0x0002,	// successful
	IPP_SUCCESSFUL_OK_E									= 0x00FF,	// successful

	IPP_INFORMATIONAL_S									= 0x0100,	// informational
	IPP_INFORMATIONAL_E									= 0x01FF,	// informational

	IPP_REDIRECTION_S									= 0x0200,	// redirection
	IPP_REDIRECTION_SE									= 0x02FF,	// redirection

	IPP_CLIENT_ERROR_S									= 0x0400,	// client-error
	IPP_CLIENT_ERROR_BAD_REQUEST						= 0x0400,	// client-error
	IPP_CLIENT_ERROR_FORBIDDEN							= 0x0401,	// client-error
	IPP_CLIENT_ERROR_NOT_AUTHENTICATED					= 0x0402,	// client-error
	IPP_CLIENT_ERROR_NOT_AUTHORIZED						= 0x0403,	// client-error
	IPP_CLIENT_ERROR_NOT_POSSIBLE						= 0x0404,	// client-error
	IPP_CLIENT_ERROR_TIMEOUT							= 0x0405,	// client-error
	IPP_CLIENT_ERROR_NOT_FOUND							= 0x0406,	// client-error
	IPP_CLIENT_ERROR_GONE								= 0x0407,	// client-error
	IPP_CLIENT_ERROR_REQUEST_ENTITY_TOO_LARGE			= 0x0408,	// client-error
	IPP_CLIENT_ERROR_REQUEST_VALUE_TOO_LONG				= 0x0409,	// client-error
	IPP_CLIENT_ERROR_DOCUMENT_FORMAT_NOT_SUPPORTED		= 0x040A,	// client-error
	IPP_CLIENT_ERROR_ATTRIBUTES_OR_VALUES_NOT_SUPPORTED	= 0x040B,	// client-error
	IPP_CLIENT_ERROR_URI_SCHEME_NOT_SUPPORTED			= 0x040C,	// client-error
	IPP_CLIENT_ERROR_CHARSET_NOT_SUPPORTED				= 0x040D,	// client-error
	IPP_CLIENT_ERROR_CONFLICTING_ATTRIBUTES				= 0x040E,	// client-error
	IPP_CLIENT_ERROR_E									= 0x04FF,	// client-error

	IPP_SERVER_ERROR_S									= 0x0500,	// server-error
	IPP_SERVER_ERROR_INTERNAL_ERROR						= 0x0500,	// server-error
	IPP_SERVER_ERROR_OPERATION_NOT_SUPPORTED			= 0x0501,	// server-error
	IPP_SERVER_ERROR_SERVICE_UNAVAILABLE				= 0x0502,	// server-error
	IPP_SERVER_ERROR_VERSION_NOT_SUPPORTED				= 0x0503,	// server-error
	IPP_SERVER_ERROR_DEVICE_ERROR						= 0x0504,	// server-error
	IPP_SERVER_ERROR_TEMPORARY_ERROR					= 0x0505,	// server-error
	IPP_SERVER_ERROR_NOT_ACCEPTING_JOBS					= 0x0506,	// server-error
	IPP_SERVER_ERROR_BUSY								= 0x0507,	// server-error
	IPP_SERVER_ERROR_JOB_CANCELED						= 0x0508,	// server-error
	IPP_SERVER_ERROR_E									= 0x05FF	// server-error
};

enum IPP_TAG {
	/* reserved: 0x00 */
	IPP_OPERATION_ATTRIBUTES_TAG	= 0x01,
	IPP_JOB_ATTRIBUTES_TAG			= 0x02,
	IPP_END_OF_ATTRIBUTES_TAG		= 0x03,
	IPP_PRINTER_ATTRIBUTES_TAG		= 0x04,
	IPP_UNSUPPORTED_ATTRIBUTES_TAG	= 0x05,
	/* reserved for future delimiters: 0x06-0x0e */
	/* reserved for future chunking-end-of-attributes-tag: 0x0F */

	IPP_UNSUPPORTED					= 0x10,
	/* reserved for future 'default': 0x11 */
	IPP_UNKNOWN						= 0x12,
	IPP_NO_VALUE					= 0x13,
	/* reserved for future "out-of-band" values: 0x14-0x1F */
	/* reserved: 0x20 */
	IPP_INTEGER						= 0x21,
	IPP_BOOLEAN						= 0x22,
	IPP_ENUM						= 0x23,
	/* reserved for future integer types: 0x24-0x2F */
	IPP_STRING						= 0x30,
	IPP_DATETIME					= 0x31,
	IPP_RESOLUTION					= 0x32,
	IPP_RANGE_OF_INTEGER			= 0x33,
	/* reserved for collection (in the future): 0x34 */
	IPP_TEXT_WITH_LANGUAGE			= 0x35,
	IPP_NAME_WITH_LANGUAGE			= 0x36,
	/* reserved for future octetString types: 0x37-0x3F */
	/* reserved: 0x40 */
	IPP_TEXT_WITHOUT_LANGUAGE		= 0x41,
	IPP_NAME_WITHOUT_LANGUAGE		= 0x42,
	/* reserved: 0x43 */
	IPP_KEYWORD						= 0x44,
	IPP_URI							= 0x45,
	IPP_URISCHEME					= 0x46,
	IPP_CHARSET						= 0x47,
	IPP_NATURAL_LANGUAGE			= 0x48,
	IPP_MIME_MEDIA_TYPE				= 0x49
	/* reserved for future character string types: 0x4A-0x5F */
};

enum IPP_RESOLUTION_UNITS {
	IPP_DOTS_PER_INCH		= 3,
	IPP_DOTS_PER_CENTIMETER	= 4
};

enum IPP_FINISHINGS {
	IPP_NONE	= 3,
	IPP_STAPLE	= 4,
	IPP_PUNCH	= 5,
	IPP_COVER	= 6,
	IPP_BIND	= 7
};

enum IPP_ORIENTATION_REQUESTED {
	IPP_PORTRAIT			= 3,
	IPP_LANDSCAPE			= 4,
	IPP_REVERSE_LANDSCAPE	= 5,
	IPP_REVERSE_PORTRAIT	= 6
};

enum IPP_PRINT_QUALITY {
	IPP_DRAFT	= 3,
	IPP_NORMAL	= 4,
	IPP_HIGH	= 5
};

enum IPP_JOB_STATE {
	IPP_JOB_STATE_PENDING			= 3,
	IPP_JOB_STATE_PENDING_HELD		= 4,
	IPP_JOB_STATE_PROCESSING		= 5,
	IPP_JOB_STATE_PROCESSING_STOPPED= 6,
	IPP_JOB_STATE_CANCELED			= 7,
	IPP_JOB_STATE_ABORTED			= 8,
	IPP_JOB_STATE_COMPLETED			= 9
};

enum IPP_PRINTER_STATE {
	IPP_PRINTER_STATEIDLE		= 3,
	IPP_PRINTER_STATEPROCESSING	= 4,
	IPP_PRINTER_STATESTOPPED	= 5
};


class IppAttribute {
public:
	IppAttribute(IPP_TAG);
	virtual ~IppAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppAttribute &attr)
	{
		return attr.output(os);
	}

	IPP_TAG tag;
};

class IppNamedAttribute : public IppAttribute {
public:
	IppNamedAttribute(IPP_TAG t);
	IppNamedAttribute(IPP_TAG t, const char *n);
	virtual ~IppNamedAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	string name;
	friend istream& operator >> (istream &is, IppNamedAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppNamedAttribute &attr)
	{
		return attr.output(os);
	}
	virtual ostream &print(ostream &) const;
};

class IppNoValueAttribute : public IppNamedAttribute {
public:
	IppNoValueAttribute(IPP_TAG t);
	IppNoValueAttribute(IPP_TAG t, const char *n);
	virtual ~IppNoValueAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppNoValueAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppNoValueAttribute &attr)
	{
		return attr.output(os);
	}
};

class IppBooleanAttribute : public IppNamedAttribute {
public:
	IppBooleanAttribute(IPP_TAG t);
	IppBooleanAttribute(IPP_TAG t, const char *n, bool f);
	virtual ~IppBooleanAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppBooleanAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppBooleanAttribute &attr)
	{
		return attr.output(os);
	}

	bool value;
};

class IppIntegerAttribute : public IppNamedAttribute {
public:
	IppIntegerAttribute(IPP_TAG t);
	IppIntegerAttribute(IPP_TAG t, const char *n, int v);
	virtual ~IppIntegerAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppIntegerAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppIntegerAttribute &attr)
	{
		return attr.output(os);
	}

	long value;
};

class DATETIME {
public:
	DATETIME();
	DATETIME(const DATETIME &);
	DATETIME & operator = (const DATETIME &);
	friend istream& operator >> (istream &is, DATETIME &attr);
	friend ostream& operator << (ostream &os, const DATETIME &attr);

	unsigned char datetime[11];
};

class IppDatetimeAttribute : public IppNamedAttribute {
public:
	IppDatetimeAttribute(IPP_TAG t);
	IppDatetimeAttribute(IPP_TAG t, const char *n, const DATETIME *dt);
	virtual ~IppDatetimeAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppDatetimeAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppDatetimeAttribute &attr)
	{
		return attr.output(os);
	}

	DATETIME datetime;
};

class IppStringAttribute : public IppNamedAttribute {
public:
	IppStringAttribute(IPP_TAG t);
	IppStringAttribute(IPP_TAG t, const char *s, const char *s1);
	virtual ~IppStringAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppStringAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppStringAttribute &attr)
	{
		return attr.output(os);
	}

	string text;
};

class IppDoubleStringAttribute : public IppNamedAttribute {
public:
	IppDoubleStringAttribute(IPP_TAG t);
	IppDoubleStringAttribute(IPP_TAG t, const char *n, const char *s1, const char *s2);
	virtual ~IppDoubleStringAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	friend istream& operator >> (istream &is, IppDoubleStringAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppDoubleStringAttribute &attr)
	{
		return attr.output(os);
	}
	virtual ostream &print(ostream &) const;

	string text1;
	string text2;
};

class IppResolutionAttribute : public IppNamedAttribute {
public:
	IppResolutionAttribute(IPP_TAG t);
	IppResolutionAttribute(IPP_TAG t, const char *n, int, int, IPP_RESOLUTION_UNITS);
	virtual ~IppResolutionAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppResolutionAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppResolutionAttribute &attr)
	{
		return attr.output(os);
	}

	int xres;
	int yres;
	IPP_RESOLUTION_UNITS resolution_units;
};

class IppRangeOfIntegerAttribute : public IppNamedAttribute {
public:
	IppRangeOfIntegerAttribute(IPP_TAG t);
	IppRangeOfIntegerAttribute(IPP_TAG t, const char *n, int, int);
	virtual ~IppRangeOfIntegerAttribute() {}
	virtual int length() const;
	virtual istream &input(istream &is);
	virtual ostream &output(ostream &os) const;
	virtual ostream &print(ostream &) const;
	friend istream& operator >> (istream &is, IppRangeOfIntegerAttribute &attr)
	{
		return attr.input(is);
	}
	friend ostream& operator << (ostream &os, const IppRangeOfIntegerAttribute &attr)
	{
		return attr.output(os);
	}

	long lower;
	long upper;
};

class IppContent {
public:
	IppContent();
	~IppContent();
	int length() const;
	istream &input(istream &);
	ostream &output(ostream &) const;
	friend istream& operator >> (istream &is, IppContent &ic)
	{
		return ic.input(is);
	}
	friend ostream& operator << (ostream &os, const IppContent &ic)
	{
		return ic.output(os);
	}
	void setVersion(unsigned short);
	unsigned short getVersion() const;
	void setOperationId(IPP_OPERATION_ID);
	IPP_OPERATION_ID getOperationId() const;
	void setRequestId(unsigned long);
	unsigned long getRequestId() const;
	IPP_STATUS_CODE getStatusCode() const;
	const char *getStatusMessage() const;

	void setDelimiter(IPP_TAG tag);
	void setInteger(const char *name, int value);
	void setBoolean(const char *name, bool value);
	void setString(const char *name, const char *value);
	void setDateTime(const char *name, const DATETIME *dt);
	void setResolution(const char *name, int x, int y, IPP_RESOLUTION_UNITS u);
	void setRangeOfInteger(const char *name, int lower, int upper);
	void setTextWithLanguage(const char *name, const char *s1, const char *s2);
	void setNameWithLanguage(const char *name, const char *s1, const char *s2);
	void setTextWithoutLanguage(const char *name, const char *value);
	void setNameWithoutLanguage(const char *name, const char *value);
	void setKeyword(const char *name, const char *value);
	void setURI(const char *name, const char *value);
	void setURIScheme(const char *name, const char *value);
	void setCharset(const char *name, const char *value);
	void setNaturalLanguage(const char *name, const char *value);
	void setMimeMediaType(const char *name, const char *value);

	void setRawData(const char *file, int size = -1);
	void setRawData(istream &is, int size = -1);
	ostream &print(ostream &) const;

	bool operator !() const;
	bool good() const;
	bool fail() const;

private:
	list<IppAttribute *> attrs;
	unsigned short version;
	unsigned short operation_id;
	unsigned long  request_id;
	string file_path;
	istream *is;
	int size;
};

#endif	// __IppContent_H
