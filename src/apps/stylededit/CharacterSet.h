#ifndef CHARACTER_SET_H
#define CHARACTER_SET_H

#include <SupportDefs.h>

/*! CharacterSet holds a variety of useful information about
	each character set.  This information has been derived from the
	IANA standards organization.  Since IANA provides several names
	for some encodings, this object also provides for aliases.
	
	@see http://www.iana.org/assignments/character-sets
	*/

class CharacterSetRoster;

class CharacterSet {
public:
	//! returns an id suitable for use in BFont::SetEncoding
	uint32 GetFontID() const { return id + 1; }
	
	//! returns an id suitable for use in convert_to_utf8/convert_from_utf8
	uint32 GetConversionID() const { return id; }
	
	//! returns an id for use in MIBs to identify coded character sets
	uint32 GetMIBenum() const { return MIBenum; }
	
	//! returns the standard IANA name for this character set
	const char * GetName() const { return iana_name; }
	
	//! returns a user interface friendly name for this character set
	const char * GetPrintName() const { return print_name; }
	
	//! returns the preferred MIME name for this character set
	//! or null if none exists
	const char * GetMIMEName() const { return mime_name; }
	
	//! returns an array of aliases for this character set
	//! or null if none exist
	const char * const * GetAliases(int32 & out_size) const { 
	  out_size = aliases_count;
	  return aliases;
	}
	
private:
	uint32	id;				//! id from convert_to_utf8/convert_from_utf8
	uint32	MIBenum;		//! for use in MIBs to identify coded character sets
	char	print_name[64]; //! user interface friendly name
	char	iana_name[64];	//! standard IANA name
	char	*mime_name;		//! the preferred mime name
	char	**aliases;		//! aliases for this character set
	uint32	aliases_count;	//! how many aliases are available
	uint32	padding[16];
private:
	CharacterSet();	
	friend class CharacterSetRoster;
};

/*! the CharacterSetRoster is used to obtain a CharacterSet object
    for a given encoding.    
    */

class CharacterSetRoster {
private:
	CharacterSetRoster();
public:
	static CharacterSetRoster * Roster(status_t * outError = NULL);
	static CharacterSetRoster * CurrentRoster(void);
	const CharacterSet * FindCharacterSetByFontID(uint32 id);
	const CharacterSet * FindCharacterSetByConversionID(uint32 id);
	const CharacterSet * FindCharacterSetByMIBenum(uint32 MIBenum);
	const CharacterSet * FindCharacterSetByPrintName(char * name);
	const CharacterSet * FindCharacterSetByName(char * name);
	uint32 GetCharacterSetCount() const { return character_sets_count; }
private:
	CharacterSet ** character_sets;
	uint32 character_sets_count;
};

#endif CHARACTER_SET_H
