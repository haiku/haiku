#include <CharacterSet.h>

namespace BPrivate {

BCharacterSet::BCharacterSet()
{
	id = 0;
	MIBenum = 106;
	print_name = "Unicode";
	iana_name = "UTF-8";
	mime_name = "UTF-8";
	aliases_count = 0;
	aliases = NULL;
}

BCharacterSet::BCharacterSet(uint32 _id, uint32 _MIBenum, const char * _print_name,
                             const char * _iana_name, const char * _mime_name,
                             const char ** _aliases)
{
	id = _id;
	MIBenum = _MIBenum;
	print_name = _print_name;
	iana_name = _iana_name;
	mime_name = _mime_name;
	aliases_count = 0;
	if (_aliases != 0) {
		while (_aliases[aliases_count] != 0) {
			aliases_count++;
		}
	}
	aliases = _aliases;
}

uint32
BCharacterSet::GetFontID() const
{
	return id;
}

uint32
BCharacterSet::GetConversionID() const
{
	return id-1;
}

uint32
BCharacterSet::GetMIBenum() const
{
	return MIBenum;
}

const char *
BCharacterSet::GetName() const
{
	return iana_name;
}

const char *
BCharacterSet::GetPrintName() const
{
	return print_name;
}

const char *
BCharacterSet::GetMIMEName() const
{
	return mime_name;
}

int32
BCharacterSet::CountAliases() const
{
	return aliases_count;
}

const char *
BCharacterSet::AliasAt(uint32 index) const
{
	if (index >= aliases_count) {
		return 0;
	}
	return aliases[index];
}

}
