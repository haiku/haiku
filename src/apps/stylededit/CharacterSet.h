#ifndef CHARACTER_SET_H
#define CHARACTER_SET_H

/*! CharacterSet holds a variety of useful information about
	each character set.  This information has been derived from the
	IANA standards organization.  Since IANA provides several names
	for some encodings, this object also provides for aliases.
	
	@see http://www.iana.org/assignments/character-sets
	*/

class CharacterSet {
public:
	CharacterSet();
	
	//! returns an id suitable for use in BFont::SetEncoding
	uint32 GetFontID() { return id + 1; }
	
	//! returns an id suitable for use in convert_to_utf8/convert_from_utf8
	uint32 GetConversionID() { return id; }
	
	//! returns an id for use in MIBs to identify coded character sets
	uint32 GetMIBenum() { return MIBenum; }
	
	//! returns the standard IANA name for this character set
	const char * GetName() { return iana_name; }
	
	//! returns a user interface friendly name for this character set
	const char * GetPrintName() { return print_name; }
	
	//! returns the preferred MIME name for this character set
	//! or null if none exists
	const char * GetMIMEName() { return mime_name; }
	
	//! returns an array of aliases for this character set
	//! or null if none exist
	const char ** GetAliases(int32 & out_size) { 
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
};

/*! the CharacterSetRoster is used to obtain a CharacterSet object
    for a given encoding.    
    */

class CharacterSetRoster {
public:
	const CharacterSet * FindCharacterSetByFontID(uint32 id);
	const CharacterSet * FindCharacterSetByConversionID(uint32 id);
	const CharacterSet * FindCharacterSetByMIBenum(uint32 MIBenum);
	const CharacterSet * FindCharacterSetByName(char * name);
private:
	CharacterSet * character_sets;
};

#endif CHARACTER_SET_H
