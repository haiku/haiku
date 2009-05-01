/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Collator.h>
#include <UnicodeChar.h>
#include <String.h>
#include <Message.h>

#include <ctype.h>


/**	This implements a German DIN-2 collator. It replaces German umlauts
 *	like "ä" with "ae", or "ö" with "oe", etc.
 *	For all other characters, it does the same as its parent class,
 *	BCollatorAddOn.
 *	This method is intended for sorting names (while DIN-1 is intended
 *	for words, BCollatorAddOn is already compatible with that method).
 *	It is used in German telephone books, for example.
 */

class CollatorDeutsch : public BCollatorAddOn {
	public:
		CollatorDeutsch();
		CollatorDeutsch(BMessage *archive);
		~CollatorDeutsch();
		
		// (un-)archiving API
		virtual status_t Archive(BMessage *archive, bool deep) const;
		static BArchivable *Instantiate(BMessage *archive);

	protected:
		virtual uint32 GetNextChar(const char **string, input_context &context);
};


static const char *kSignature = "application/x-vnd.locale-collator.germanDIN-2";


CollatorDeutsch::CollatorDeutsch()
{
}


CollatorDeutsch::CollatorDeutsch(BMessage *archive)
	: BCollatorAddOn(archive)
{
}


CollatorDeutsch::~CollatorDeutsch()
{
}


uint32
CollatorDeutsch::GetNextChar(const char **string, input_context &context)
{
	uint32 c = context.next_char;
	if (c != 0) {
		context.next_char = 0;
		return c;
	}

	do {
		c = BUnicodeChar::FromUTF8(string);
	} while (context.ignore_punctuation
		&& (BUnicodeChar::IsPunctuation(c) || BUnicodeChar::IsSpace(c)));

	switch (c) {
		case 223:	// ß
			context.next_char = 's';
			return 's';
		case 196:	// Ae
			context.next_char = 'e';
			return 'A';
		case 214:	// Oe
			context.next_char = 'e';
			return 'O';
		case 220:	// Ue
			context.next_char = 'e';
			return 'U';
		case 228:	// ae
			context.next_char = 'e';
			return 'a';
		case 246:	// oe
			context.next_char = 'e';
			return 'o';
		case 252:	// ue
			context.next_char = 'e';
			return 'u';
	}

	return c;
}


status_t 
CollatorDeutsch::Archive(BMessage *archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	// add the add-on signature, so that the roster can load
	// us on demand!
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);

	return status;
}


BArchivable *
CollatorDeutsch::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "CollatorDeutsch"))
		return new CollatorDeutsch(archive);

	return NULL;
}


//	#pragma mark -


extern "C" BCollatorAddOn *
instantiate_collator(void)
{
	return new CollatorDeutsch();
}
