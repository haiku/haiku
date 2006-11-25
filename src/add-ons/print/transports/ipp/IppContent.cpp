// Sun, 18 Jun 2000
// Y.Takagi

#if defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_BONE)
#	include <sys/socket.h>
#	include <netinet/in.h>
#else
#	include <net/socket.h>
#endif

#include <fstream>
#include <list>
#include <cstring>

#include "IppContent.h"

/*----------------------------------------------------------------------*/

short readLength(istream &is)
{
	short len = 0;
	is.read((char *)&len, sizeof(short));
	len = ntohs(len);
	return len;
}

void writeLength(ostream &os, short len)
{
	len = htons(len);
	os.write((char *)&len, sizeof(short));
}

/*----------------------------------------------------------------------*/

DATETIME::DATETIME()
{
	memset(this, 0, sizeof(DATETIME));
}

DATETIME::DATETIME(const DATETIME &dt)
{
	memcpy(this, &dt.datetime, sizeof(DATETIME));
}

DATETIME & DATETIME::operator = (const DATETIME &dt)
{
	memcpy(this, &dt.datetime, sizeof(DATETIME));
	return *this;
}

istream& operator >> (istream &is, DATETIME &attr)
{
	return is;
}

ostream& operator << (ostream &os, const DATETIME &attr)
{
	return os;
}


/*----------------------------------------------------------------------*/

IppAttribute::IppAttribute(IPP_TAG t)
	: tag(t)
{
}

int IppAttribute::length() const
{
	return 1;
}

istream &IppAttribute::input(istream &is)
{
	return is;
}

ostream &IppAttribute::output(ostream &os) const
{
	os << (unsigned char)tag;
	return os;
}

ostream &IppAttribute::print(ostream &os) const
{
	os << "Tag: " << hex << (int)tag << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppNamedAttribute::IppNamedAttribute(IPP_TAG t)
	: IppAttribute(t)
{
}

IppNamedAttribute::IppNamedAttribute(IPP_TAG t, const char *s)
	: IppAttribute(t), name(s ? s : "")
{
}

int IppNamedAttribute::length() const
{
	return IppAttribute::length() + 2 + name.length();
}

istream &IppNamedAttribute::input(istream &is)
{
	short len = readLength(is);

	if (0 < len) {
		char *buffer = new char[len + 1];
		is.read(buffer, len);
		buffer[len] = '\0';
		name = buffer;
		delete [] buffer;
	}

	return is;
}

ostream &IppNamedAttribute::output(ostream &os) const
{
	IppAttribute::output(os);

	writeLength(os, name.length());
	os << name;

	return os;
}

ostream &IppNamedAttribute::print(ostream &os) const
{
	IppAttribute::print(os);
	os << '\t' << "Name: " << name << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppNoValueAttribute::IppNoValueAttribute(IPP_TAG t)
	: IppNamedAttribute(t)
{
}

IppNoValueAttribute::IppNoValueAttribute(IPP_TAG t, const char *n)
	: IppNamedAttribute(t, n)
{
}

int IppNoValueAttribute::length() const
{
	return IppAttribute::length() + 2;
}

istream &IppNoValueAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len) {
		is.seekg(len, ios::cur);
	}

	return is;
}

ostream &IppNoValueAttribute::output(ostream &os) const
{
	IppAttribute::output(os);

	writeLength(os, 0);

	return os;
}

ostream &IppNoValueAttribute::print(ostream &os) const
{
	return IppNamedAttribute::print(os);
}

/*----------------------------------------------------------------------*/

IppIntegerAttribute::IppIntegerAttribute(IPP_TAG t)
	: IppNamedAttribute(t), value(0)
{
}

IppIntegerAttribute::IppIntegerAttribute(IPP_TAG t, const char *n, int v)
	: IppNamedAttribute(t, n), value(v)
{
}

int IppIntegerAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + 4;
}

istream &IppIntegerAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len && len <= 4) {
		is.read((char *)&value, sizeof(value));
		value = ntohl(value);
	} else {
		is.seekg(len, ios::cur);
	}

	return is;
}

ostream &IppIntegerAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, 4);
	unsigned long val = htonl(value);
	os.write((char *)&val, sizeof(val));
	return os;
}

ostream &IppIntegerAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value: " << dec << value << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppBooleanAttribute::IppBooleanAttribute(IPP_TAG t)
	: IppNamedAttribute(t), value(false)
{
}

IppBooleanAttribute::IppBooleanAttribute(IPP_TAG t, const char *n, bool f)
	: IppNamedAttribute(t, n), value(f)
{
}

int IppBooleanAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + 1;
}


istream &IppBooleanAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len && len <= 1) {
		char c;
		is.read((char *)&c, sizeof(c));
		value = c ? true : false;
	} else {
		is.seekg(len, ios::cur);
	}

	return is;
}

ostream &IppBooleanAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, 1);
	char c = (char)value;
	os.write((char *)&c, sizeof(c));

	return os;
}

ostream &IppBooleanAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value: " << value << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppDatetimeAttribute::IppDatetimeAttribute(IPP_TAG t)
	: IppNamedAttribute(t)
{
}

IppDatetimeAttribute::IppDatetimeAttribute(IPP_TAG t, const char *n, const DATETIME *dt)
	: IppNamedAttribute(t, n), datetime(*dt)
{
}

int IppDatetimeAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + 11;
}

istream &IppDatetimeAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len) {
		if (len == 11) {
			is >> datetime;
		} else {
			is.seekg(len, ios::cur);
		}
	}

	return is;
}

ostream &IppDatetimeAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, 11);
	os << datetime;

	return os;
}

ostream &IppDatetimeAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value(DateTime): " << datetime << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppStringAttribute::IppStringAttribute(IPP_TAG t)
	: IppNamedAttribute(t)
{
}

IppStringAttribute::IppStringAttribute(IPP_TAG t, const char *n, const char *s)
: IppNamedAttribute(t, n), text(s ? s : "")
{
}

int IppStringAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + text.length();
}

istream &IppStringAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len) {
		char *buffer = new char[len + 1];
		is.read(buffer, len);
		buffer[len] = '\0';
		text = buffer;
		delete [] buffer;
	}

	return is;
}

ostream &IppStringAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, text.length());
	os << text;

	return os;
}

ostream &IppStringAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value: " << text << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppDoubleStringAttribute::IppDoubleStringAttribute(IPP_TAG t)
	: IppNamedAttribute(t)
{
}

IppDoubleStringAttribute::IppDoubleStringAttribute(IPP_TAG t, const char *n, const char *s1, const char *s2)
: IppNamedAttribute(t, n), text1(s1 ? s1 : ""), text2(s2 ? s2 : "")
{
}

int IppDoubleStringAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + text1.length() + 2  + text2.length();
}

istream &IppDoubleStringAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len) {
		char *buffer = new char[len + 1];
		is.read(buffer, len);
		buffer[len] = '\0';
		text1 = buffer;
		delete [] buffer;
	}

	len = readLength(is);

	if (0 < len) {
		char *buffer = new char[len + 1];
		is.read(buffer, len);
		buffer[len] = '\0';
		text2 = buffer;
		delete [] buffer;
	}

	return is;
}

ostream &IppDoubleStringAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, text1.length());
	os << text1;

	writeLength(os, text2.length());
	os << text2;

	return os;
}

ostream &IppDoubleStringAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value1: " << text1 << '\n';
	os << '\t' << "Value2: " << text2 << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppResolutionAttribute::IppResolutionAttribute(IPP_TAG t)
	: IppNamedAttribute(t), xres(0), yres(0), resolution_units((IPP_RESOLUTION_UNITS)0)
{
}

IppResolutionAttribute::IppResolutionAttribute(IPP_TAG t, const char *n, int x, int y, IPP_RESOLUTION_UNITS u)
	: IppNamedAttribute(t, n), xres(x), yres(y), resolution_units(u)
{
}

int IppResolutionAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + 4 + 2 + 4 + 2 + 1;
}

istream &IppResolutionAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len && len <= 4) {
		is.read((char *)&xres, sizeof(xres));
		xres = ntohl(xres);
	} else {
		is.seekg(len, ios::cur);
	}

	len = readLength(is);

	if (0 < len && len <= 4) {
		is.read((char *)&yres, sizeof(yres));
		yres = ntohl(yres);
	} else {
		is.seekg(len, ios::cur);
	}

	len = readLength(is);

	if (len == 1) {
		char c;
		is.read((char *)&c, sizeof(c));
		resolution_units = (IPP_RESOLUTION_UNITS)c;
	} else {
		is.seekg(len, ios::cur);
	}

	return is;
}

ostream &IppResolutionAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, 4);
	unsigned long val = htonl(xres);
	os.write((char *)&val, sizeof(val));

	writeLength(os, 4);
	val = htonl(yres);
	os.write((char *)&val, sizeof(val));

	writeLength(os, 1);
	unsigned char c = (unsigned char)resolution_units;
	os.write((char *)&c, sizeof(c));

	return os;
}

ostream &IppResolutionAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value(xres): " << dec << xres << '\n';
	os << '\t' << "Value(yres): " << dec << yres << '\n';
	os << '\t' << "Value(unit): " << dec << resolution_units << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppRangeOfIntegerAttribute::IppRangeOfIntegerAttribute(IPP_TAG t)
	: IppNamedAttribute(t), lower(0), upper(0)
{
}

IppRangeOfIntegerAttribute::IppRangeOfIntegerAttribute(IPP_TAG t, const char *n, int l, int u)
	: IppNamedAttribute(t, n), lower(l), upper(u)
{
}

int IppRangeOfIntegerAttribute::length() const
{
	return IppNamedAttribute::length() + 2 + 4 + 2 + 4;
}

istream &IppRangeOfIntegerAttribute::input(istream &is)
{
	IppNamedAttribute::input(is);

	short len = readLength(is);

	if (0 < len && len <= 4) {
		is.read((char *)&lower, sizeof(lower));
		lower = ntohl(lower);
	} else {
		is.seekg(len, ios::cur);
	}

	len = readLength(is);

	if (0 < len && len <= 4) {
		is.read((char *)&upper, sizeof(upper));
		upper = ntohl(upper);
	} else {
		is.seekg(len, ios::cur);
	}

	return is;
}

ostream &IppRangeOfIntegerAttribute::output(ostream &os) const
{
	IppNamedAttribute::output(os);

	writeLength(os, 4);
	unsigned long val = htonl(lower);
	os.write((char *)&val, sizeof(val));

	writeLength(os, 4);
	val = htonl(upper);
	os.write((char *)&val, sizeof(val));

	return os;
}

ostream &IppRangeOfIntegerAttribute::print(ostream &os) const
{
	IppNamedAttribute::print(os);
	os << '\t' << "Value(lower): " << dec << lower << '\n';
	os << '\t' << "Value(upper): " << dec << upper << '\n';
	return os;
}

/*----------------------------------------------------------------------*/

IppContent::IppContent()
{
	version      = 0x0100;
	operation_id = IPP_GET_PRINTER_ATTRIBUTES;
	request_id   = 0x00000001;

	is = NULL;
	size = -1;
}

IppContent::~IppContent()
{
	for (list<IppAttribute *>::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
		delete (*it);
	}
}

unsigned short IppContent::getVersion() const
{
	return version;
}

void IppContent::setVersion(unsigned short i)
{
	version = i;
}


IPP_OPERATION_ID IppContent::getOperationId() const
{
	return (IPP_OPERATION_ID)operation_id;
}

void IppContent::setOperationId(IPP_OPERATION_ID i)
{
	operation_id = i;
}

IPP_STATUS_CODE IppContent::getStatusCode() const
{
	return (IPP_STATUS_CODE)operation_id;
}

unsigned long IppContent::getRequestId() const
{
	return request_id;
}

void IppContent::setRequestId(unsigned long i)
{
	request_id = i;
}

istream &IppContent::input(istream &is)
{
	if (!is.read((char *)&version, sizeof(version))) {
		return is;
	}

	version = ntohs(version);

	if (!is.read((char *)&operation_id, sizeof(operation_id))) {
		return is;
	}

	operation_id = ntohs(operation_id);

	if (!is.read((char *)&request_id, sizeof(request_id))) {
		return is;
	}

	request_id = ntohl(request_id);
	char tag;

	while (1) {

		if (!is.read((char *)&tag, sizeof(tag))) {
			return is;
		}

		if (tag <= 0x0F) {	// delimiter

//			case IPP_OPERATION_ATTRIBUTES_TAG:
//			case IPP_JOB_ATTRIBUTES_TAG:
//			case IPP_END_OF_ATTRIBUTES_TAG:
//			case IPP_PRINTER_ATTRIBUTES_TAG:
//			case IPP_UNSUPPORTED_ATTRIBUTES_TAG:

			attrs.push_back(new IppAttribute((IPP_TAG)tag));
			if (tag == IPP_END_OF_ATTRIBUTES_TAG) {
				break;
			}

		} else if (tag <= 0x1F) {

			IppNoValueAttribute *attr = new IppNoValueAttribute((IPP_TAG)tag);
			is >> *attr;
			attrs.push_back(attr);

		} else if (tag <= 0x2F) {	// integer values

			switch (tag) {
			case IPP_INTEGER:
			case IPP_ENUM:
				{
					IppIntegerAttribute *attr = new IppIntegerAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			case IPP_BOOLEAN:
				{
					IppBooleanAttribute *attr = new IppBooleanAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			default:
				{
					short len = readLength(is);
					is.seekg(len, ios::cur);
					len = readLength(is);
					is.seekg(len, ios::cur);
				}
				break;
			}

		} else if (tag <= 0x3F) {	// octetString values

			switch (tag) {
			case IPP_STRING:
				{
					IppStringAttribute *attr = new IppStringAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			case IPP_DATETIME:
				{
					IppDatetimeAttribute *attr = new IppDatetimeAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			case IPP_RESOLUTION:
				{
					IppResolutionAttribute *attr = new IppResolutionAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			case IPP_RANGE_OF_INTEGER:
				{
					IppRangeOfIntegerAttribute *attr = new IppRangeOfIntegerAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			case IPP_TEXT_WITH_LANGUAGE:
			case IPP_NAME_WITH_LANGUAGE:
				{
					IppDoubleStringAttribute *attr = new IppDoubleStringAttribute((IPP_TAG)tag);
					is >> *attr;
					attrs.push_back(attr);
				}
				break;
			default:
				{
					short len = readLength(is);
					is.seekg(len, ios::cur);
					len = readLength(is);
					is.seekg(len, ios::cur);
				}
				break;
			}

		} else if (tag <= 0x5F) {	// character-string values

//			case IPP_TEXT_WITHOUT_LANGUAGE:
//			case IPP_NAME_WITHOUT_LANGUAGE:
//			case IPP_KEYWORD:
//			case IPP_URI:
//			case IPP_URISCHEME:
//			case IPP_CHARSET:
//			case IPP_NATURAL_LANGUAGE:
//			case IPP_MIME_MEDIA_TYPE:

			IppStringAttribute *attr = new IppStringAttribute((IPP_TAG)tag);
			is >> *attr;
			attrs.push_back(attr);
		}
	}
	return is;
}

ostream &IppContent::output(ostream &os) const
{
	unsigned short ns_version = htons(version);						// version-number
	os.write((char *)&ns_version, sizeof(ns_version));				// version-number

	unsigned short ns_operation_id = htons(operation_id);			// operation-id
	os.write((char *)&ns_operation_id, sizeof(ns_operation_id));	// operation-id

	unsigned long ns_request_id = htonl(request_id);				// request-id
	os.write((char *)&ns_request_id, sizeof(ns_request_id));		// request-id

	for (list<IppAttribute *>::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
		os << *(*it);
	}

	ifstream ifs;
	istream *iss = is;
	if (iss == NULL) {
		if (!file_path.empty()) {
			ifs.open(file_path.c_str(), ios::in | ios::binary);
			iss = &ifs;
		}
	}
	if (iss && iss->good()) {
		if (iss->good()) {
			char c;
			while (iss->get(c)) {
				os.put(c);
			}
		}
	}

	return os;
}

void IppContent::setDelimiter(IPP_TAG tag)
{
	attrs.push_back(new IppAttribute(tag));
}

void IppContent::setInteger(const char *name, int value)
{
	attrs.push_back(new IppIntegerAttribute(IPP_INTEGER, name, value));
}

void IppContent::setBoolean(const char *name, bool value)
{
	attrs.push_back(new IppBooleanAttribute(IPP_BOOLEAN, name, value));
}

void IppContent::setString(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_STRING, name, value));
}

void IppContent::setDateTime(const char *name, const DATETIME *dt)
{
	attrs.push_back(new IppDatetimeAttribute(IPP_DATETIME, name, dt));
}

void IppContent::setResolution(const char *name, int x, int y, IPP_RESOLUTION_UNITS u)
{
	attrs.push_back(new IppResolutionAttribute(IPP_RESOLUTION, name, x, y, u));
}

void IppContent::setRangeOfInteger(const char *name, int lower, int upper)
{
	attrs.push_back(new IppRangeOfIntegerAttribute(IPP_RANGE_OF_INTEGER, name, lower, upper));
}

void IppContent::setTextWithLanguage(const char *name, const char *s1, const char *s2)
{
	attrs.push_back(new IppDoubleStringAttribute(IPP_TEXT_WITH_LANGUAGE, name, s1, s2));
}

void IppContent::setNameWithLanguage(const char *name, const char *s1, const char *s2)
{
	attrs.push_back(new IppDoubleStringAttribute(IPP_NAME_WITH_LANGUAGE, name, s1, s2));
}

void IppContent::setTextWithoutLanguage(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_TEXT_WITHOUT_LANGUAGE, name, value));
}

void IppContent::setNameWithoutLanguage(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_NAME_WITHOUT_LANGUAGE, name, value));
}

void IppContent::setKeyword(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_KEYWORD, name, value));
}

void IppContent::setURI(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_URI, name, value));
}

void IppContent::setURIScheme(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_URISCHEME, name, value));
}

void IppContent::setCharset(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_CHARSET, name, value));
}

void IppContent::setNaturalLanguage(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_NATURAL_LANGUAGE, name, value));
}

void IppContent::setMimeMediaType(const char *name, const char *value)
{
	attrs.push_back(new IppStringAttribute(IPP_MIME_MEDIA_TYPE, name, value));
}

int IppContent::length() const
{
	int length = 8;	// sizeof(version-number + operation-id + request-id)

	for (list<IppAttribute *>::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
		length += (*it)->length();
	}

	ifstream ifs;
	istream *iss = is;
	if (iss == NULL) {
		if (!file_path.empty()) {
			ifs.open(file_path.c_str(), ios::in | ios::binary);
			iss = &ifs;
		}
	}
	if (iss && iss->good()) {
		int fsize = size;
		if (fsize < 0) {
			streampos pos = iss->tellg();
			iss->seekg(0, ios::end);
			fsize = iss->tellg();
			iss->seekg(pos, ios::beg);
		}
		if (fsize > 0) {
			length += fsize;
		}
	}

	return length;
}

void IppContent::setRawData(const char *file, int n)
{
	file_path = file;
	size = n;
}

void IppContent::setRawData(istream &ifs, int n)
{
	is = &ifs;
	size = n;
}

ostream &IppContent::print(ostream &os) const
{
	os << "version:      " << hex << version << '\n';
	os << "operation_id: " << hex << operation_id << '\n';
	os << "request_id:   " << hex << request_id << '\n';

	for (list<IppAttribute *>::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
		(*it)->print(os);
	}

	return os;
}

bool IppContent::fail() const
{
	return !good();
}

bool IppContent::good() const
{
	return /*operation_id >= IPP_SUCCESSFUL_OK_S &&*/ operation_id <= IPP_SUCCESSFUL_OK_E;
}

bool IppContent::operator !() const
{
	return fail();
}

const char *IppContent::getStatusMessage() const
{
	if (good()) {
		switch (operation_id) {
		case IPP_SUCCESSFUL_OK:
			return "successful-ok";
		case IPP_SUCCESSFUL_OK_IGNORED_OR_SUBSTITUTED_ATTRIBUTES:
			return "successful-ok-ignored-or-substituted-attributes";
		case IPP_SUCCESSFUL_OK_CONFLICTING_ATTRIBUTES:
			return "successful-ok-conflicting-attributes";
		default:
			return "successful-ok-???";
		}
	} else if (IPP_CLIENT_ERROR_S <= operation_id && operation_id <= IPP_CLIENT_ERROR_E) {
		switch (operation_id) {
		case IPP_CLIENT_ERROR_BAD_REQUEST:
			return "client-error-bad-request";
		case IPP_CLIENT_ERROR_FORBIDDEN:
			return "client-error-forbidden";
		case IPP_CLIENT_ERROR_NOT_AUTHENTICATED:
			return "client-error-not-authenticated";
		case IPP_CLIENT_ERROR_NOT_AUTHORIZED:
			return "client-error-not-authorized";
		case IPP_CLIENT_ERROR_NOT_POSSIBLE:
			return "client-error-not-possible";
		case IPP_CLIENT_ERROR_TIMEOUT:
			return "client-error-timeout";
		case IPP_CLIENT_ERROR_NOT_FOUND:
			return "client-error-not-found";
		case IPP_CLIENT_ERROR_GONE:
			return "client-error-gone";
		case IPP_CLIENT_ERROR_REQUEST_ENTITY_TOO_LARGE:
			return "client-error-request-entity-too-large";
		case IPP_CLIENT_ERROR_REQUEST_VALUE_TOO_LONG:
			return "client-error-request-value-too-long";
		case IPP_CLIENT_ERROR_DOCUMENT_FORMAT_NOT_SUPPORTED:
			return "client-error-document-format-not-supported";
		case IPP_CLIENT_ERROR_ATTRIBUTES_OR_VALUES_NOT_SUPPORTED:
			return "client-error-attributes-or-values-not-supported";
		case IPP_CLIENT_ERROR_URI_SCHEME_NOT_SUPPORTED:
			return "client-error-uri-scheme-not-supported";
		case IPP_CLIENT_ERROR_CHARSET_NOT_SUPPORTED:
			return "client-error-charset-not-supported";
		case IPP_CLIENT_ERROR_CONFLICTING_ATTRIBUTES:
			return "client-error-conflicting-attributes";
		default:
			return "client-error-???";
		}
	} else if (IPP_SERVER_ERROR_S <= operation_id && operation_id <= IPP_SERVER_ERROR_E) {
		switch (operation_id) {
		case IPP_SERVER_ERROR_INTERNAL_ERROR:
			return "server-error-internal-error";
		case IPP_SERVER_ERROR_OPERATION_NOT_SUPPORTED:
			return "server-error-operation-not-supported";
		case IPP_SERVER_ERROR_SERVICE_UNAVAILABLE:
			return "server-error-service-unavailable";
		case IPP_SERVER_ERROR_VERSION_NOT_SUPPORTED:
			return "server-error-version-not-supported";
		case IPP_SERVER_ERROR_DEVICE_ERROR:
			return "server-error-device-error";
		case IPP_SERVER_ERROR_TEMPORARY_ERROR:
			return "server-error-temporary-error";
		case IPP_SERVER_ERROR_NOT_ACCEPTING_JOBS:
			return "server-error-not-accepting-jobs";
		case IPP_SERVER_ERROR_BUSY:
			return "server-error-busy";
		case IPP_SERVER_ERROR_JOB_CANCELED:
			return "server-error-job-canceled";
		default:
			return "server-error-???";
		}
	} else {
		return "unknown error.";
	}
}
