#include "BoneyardAddOn.h"

_EXPORT void boneyard_do_save() {}
_EXPORT void boneyard_do_revert() {}

BYDataEntry::BYDataEntry(const char *name, const char *value, const char *comment)
{
	m_name 		= (char *) name;
	m_value 	= (char *) value;
	m_comment 	= (char *) comment;
}
	
BYDataEntry::~BYDataEntry() {}

const char * BYDataEntry::Name() 	{ return m_name; }
const char * BYDataEntry::Value() 	{ return m_value; }
const char * BYDataEntry::Comment()	{ return m_comment; }
void BYDataEntry::SetValue(char *value) 		{ m_value = value; }
void BYDataEntry::SetComment(char *comment) 	{ m_comment = comment; }

void BYDataEntry::_ReservedDataEntry1() {}
void BYDataEntry::_ReservedDataEntry2() {}

BYAddon::BYAddon() {}
BYAddon::~BYAddon() {}
BView * BYAddon::CreateView(BRect *bounds) { return NULL; }
void BYAddon::Revert() {}
void BYAddon::Save() {}

const char *BYAddon::Name() { return "noname"; }
const char *BYAddon::Section() { return "noname"; }
const char *BYAddon::Description() { return "noname"; }
const BList *BYAddon::FileList() { return NULL; }
int BYAddon::Importance() { return 10; }

const BList *BYAddon::ObtainSectionData() { return NULL; }			// get all BYDataEntrys for my section.
const char *BYAddon::GetValue(const char *name) { return ""; }		// get a value from your section.
void BYAddon::SetValue(const char *name,const char *data) {}		// set a value in your section.
void BYAddon::ClearValue(const char *name) {}			// remove an entry from your section.
void BYAddon::ClearSectionData() {}					// remove all entries from your section.
bool BYAddon::IsDirty() { return false; }								// query dirty status
void BYAddon::SetDirty(bool _dirty) {}			// set dirty status
const BList *BYAddon::GetAddonList() { return NULL; }				// get all add-ons in the cradle.

void BYAddon::_ReservedAddon1() {}
void BYAddon::_ReservedAddon2() {}
void BYAddon::_ReservedAddon3() {}
void BYAddon::_ReservedAddon4() {}
void BYAddon::_ReservedAddon5() {}
void BYAddon::_ReservedAddon6() {}
void BYAddon::_ReservedAddon7() {}
void BYAddon::_ReservedAddon8() {}

// void BYAddon::GetAddonList();

void save_config();

