/*
 * Copyright 2002-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file SnifferRules.cpp
	SnifferRules class implementation
*/

#include "SnifferRules.h"

#include <stdio.h>
#include <sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <MimeType.h>
#include <mime/database_support.h>
#include <sniffer/Parser.h>
#include <sniffer/Rule.h>
#include <StorageDefs.h>
#include <storage_support.h>
#include <String.h>

#include "MimeSnifferAddonManager.h"

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

using namespace BPrivate::Storage;

/*!
	\struct SnifferRules::sniffer_rule
	\brief A parsed sniffer rule and its corresponding mime type and rule string

	The parse sniffer rule is stored in the \c rule member, which is a pointer
	to a \c Sniffer::Rule object. This design was chosen to allow \c sniffer_rule
	objects	(as opposed to \c sniffer_rule pointers) to be used with STL objects
	without unnecessary copying. As a consequence of this decision, the
	\c SnifferRules object managing the rule list is responsible for actually
	deleting each \c sniffer_rule's \c Sniffer::Rule object.
*/

// sniffer_rule Constructor
//! Creates a new \c sniffer_rule object
SnifferRules::sniffer_rule::sniffer_rule(Sniffer::Rule *rule)
	: rule(rule)
{
}

// sniffer_rule Destructor
//! Destroys the \c sniffer_rule object.
/*! \note The \c Sniffer::Rule object pointed to by the \c sniffer_rule
	object's \c rule member is *NOT* deleted by this function.
*/
SnifferRules::sniffer_rule::~sniffer_rule()
{
}

// private functions
/*! \brief Returns true if \a left's priority is greater than \a right's

	This may seem slightly backwards, but since sort() using
	operator<() sorts in ascending order, we say "left < right"
	if "left.priority > right.priority" to get them sorted in
	ascending order. Super, no?

	Also, sniffer_rule objects with \c NULL \c rule members are
	treated as having minimal priority (and thus are placed at
	the end of the list of rules).

	Finally, sniffer_rule objects that are otherwise equal are
	sorted in reverse alphabetic order (thus placing sniffer
	rules for supertypes *after* sniffer rules for subtypes
	of said supertype when both rules have identical priorities).
*/
bool operator<(SnifferRules::sniffer_rule &left, SnifferRules::sniffer_rule &right)
{
	if (left.rule && right.rule) {
		double leftPriority = left.rule->Priority();
		double rightPriority = right.rule->Priority();
		if (leftPriority > rightPriority) {
			return true;	// left < right
		} else if (rightPriority > leftPriority) {
			return false;	// right < left
		} else {
			return left.type > right.type;
		}
	} else if (left.rule) {
		return true; 	// left < right
	} else {
		return false;	// right < left
	}
}

/*!
	\class SnifferRules
	\brief Manages the sniffer rules for the entire database
*/

// Constructor
//! Constructs a new SnifferRules object
SnifferRules::SnifferRules()
	: fHaveDoneFullBuild(false)
{
}

// Destructor
/*! \brief Destroys the \c SnifferRules object and all dynamically allocated
	\c Sniffer::Rule objects scattered throughout the rule list in
	\c sniffer_rule::rule members.
*/
SnifferRules::~SnifferRules()
{
	for (std::list<sniffer_rule>::iterator i = fRuleList.begin();
		   i != fRuleList.end();
		     i++)
	{
		delete i->rule;
		i->rule = NULL;
	}
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the supplied entry_ref.

	Only the data in the given entry is considered, not the filename or
	its extension. Please see GuessMimeType(BFile*, const void *, int32,
	BString*) for more details.

	\param ref The entry to sniff
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success
	- \c Mime::kMimeGuessFailure: no match found (\a type is left unmodified)
	- error code: failure
*/
status_t
SnifferRules::GuessMimeType(const entry_ref *ref, BString *type)
{
	status_t err = ref && type ? B_OK : B_BAD_VALUE;
	ssize_t bytes = 0;
	char *buffer = NULL;
	BFile file;

	// First find out the max number of bytes we need to read
	// from the file to fully accomodate all of our currently
	// installed sniffer rules
	if (!err) {
		bytes = MaxBytesNeeded();
		if (bytes < 0)
			err = bytes;
	}

	// Next read that many bytes (or fewer, if the file isn't
	// that long) into a buffer
	if (!err) {
		buffer = new(std::nothrow) char[bytes];
		if (!buffer)
			err = B_NO_MEMORY;
	}

	if (!err)
		err = file.SetTo(ref, B_READ_ONLY);
	if (!err) {
		bytes = file.Read(buffer, bytes);
		if (bytes < 0)
			err = bytes;
	}

	// Now sniff the buffer
	if (!err)
		err = GuessMimeType(&file, buffer, bytes, type);

	delete[] buffer;

	return err;
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the given chunk of data.

	Please see GuessMimeType(BFile*, const void *, int32, BString*) for more
	details.

	\param buffer Pointer to a data buffer to sniff
	\param length The length of the data buffer pointed to by \a buffer
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success
	- \c Mime::kMimeGuessFailure: no match found (\a type is left unmodified)
	- error code: failure
*/
status_t
SnifferRules::GuessMimeType(const void *buffer, int32 length, BString *type)
{
	return GuessMimeType(NULL, buffer, length, type);
}

// SetSnifferRule
/*! Updates the sniffer rule for the given type

	If the a rule currently exists in the rule list for the given type,
	it is first removed before the new rule is inserted.

	The new rule is inserted in its proper, sorted position in the list.

	\param type The type of interest
	\param rule The new sniffer rule
	\return
	- \c B_OK: success
	- other error code: failure
*/
status_t
SnifferRules::SetSnifferRule(const char *type, const char *rule)
{
	status_t err = type && rule ? B_OK : B_BAD_VALUE;
	if (!err && !fHaveDoneFullBuild)
		return B_OK;

	sniffer_rule item(new Sniffer::Rule());
	BString parseError;

	// Check the mem alloc
	if (!err)
		err = item.rule ? B_OK : B_NO_MEMORY;
	// Prepare the sniffer_rule
	if (!err) {
		item.type = type;
		item.rule_string = rule;
		err = Sniffer::parse(rule, item.rule, &parseError);
		if (err)
			DBG(OUT("ERROR: SnifferRules::SetSnifferRule(): rule parsing error:\n%s\n",
				parseError.String()));
	}
	// Remove any previous rule for this type
	if (!err)
		err = DeleteSnifferRule(type);
	// Insert the new rule at the proper position in
	// the sorted rule list (remembering that our list
	// is sorted in ascending order using
	// operator<(sniffer_rule&, sniffer_rule&))
	if (!err) {
		std::list<sniffer_rule>::iterator i;
		for (i = fRuleList.begin(); i != fRuleList.end(); i++)
		{
			 if (item < (*i)) {
			 	fRuleList.insert(i, item);
			 	break;
			 }
		}
		if (i == fRuleList.end())
			fRuleList.push_back(item);
	}

	return err;
}

// DeleteSnifferRule
/*! \brief Removes the sniffer rule for the given type from the rule list
	\param type The type of interest
	\return
	- \c B_OK: success (even if no rule existed for the given type)
	- other error code: failure
*/
status_t
SnifferRules::DeleteSnifferRule(const char *type)
{
	status_t err = type ? B_OK : B_BAD_VALUE;
	if (!err && !fHaveDoneFullBuild)
		return B_OK;

	// Find the rule in the list and remove it
	for (std::list<sniffer_rule>::iterator i = fRuleList.begin();
		   i != fRuleList.end();
		     i++)
	{
		if (i->type == type) {
			fRuleList.erase(i);
			break;
		}
	}

	return err;
}

// PrintToStream
//! Dumps the list of sniffer rules in sorted order to standard output
void
SnifferRules::PrintToStream() const
{
	printf("\n");
	printf("--------------\n");
	printf("Sniffer Rules:\n");
	printf("--------------\n");

	if (fHaveDoneFullBuild) {
		for (std::list<sniffer_rule>::const_iterator i = fRuleList.begin();
			   i != fRuleList.end();
			     i++)
		{
			printf("%s: '%s'\n", i->type.c_str(), i->rule_string.c_str());
		}
	} else {
		printf("You haven't built your rule list yet, chump. ;-)\n");
	}
}

// BuildRuleList
/*! \brief Crawls through the database, parses each sniffer rule it finds, adds
	each parsed rule to the rule list, and sorts the list by priority, largest first.

	Initial MaxBytesNeeded() info is compiled by this function as well.
*/
status_t
SnifferRules::BuildRuleList()
{
	fRuleList.clear();

	ssize_t maxBytesNeeded = 0;
	ssize_t bytesNeeded = 0;
	BDirectory root;

	status_t err = root.SetTo(get_database_directory().c_str());
	if (!err) {
		root.Rewind();
		while (true) {
			BEntry entry;
			err = root.GetNextEntry(&entry);
			if (err) {
				// If we've come to the end of list, it's not an error
				if (err == B_ENTRY_NOT_FOUND)
					err = B_OK;
				break;
			} else {
				// Check that this entry is both a directory and a valid MIME string
				char supertype[B_PATH_NAME_LENGTH];
				if (entry.IsDirectory()
				      && entry.GetName(supertype) == B_OK
				         && BMimeType::IsValid(supertype))
				{
					// Make sure the supertype string is all lowercase
					BPrivate::Storage::to_lower(supertype);

					// First, iterate through this supertype directory and process
					// all of its subtypes
					BDirectory dir;
					if (dir.SetTo(&entry) == B_OK) {
						dir.Rewind();
						while (true) {
							BEntry subEntry;
							err = dir.GetNextEntry(&subEntry);
							if (err) {
								// If we've come to the end of list, it's not an error
								if (err == B_ENTRY_NOT_FOUND)
									err = B_OK;
								break;
							} else {
								// Get the subtype's name
								char subtype[B_PATH_NAME_LENGTH];
								if (subEntry.GetName(subtype) == B_OK) {
									BPrivate::Storage::to_lower(subtype);

									char fulltype[B_PATH_NAME_LENGTH];
									sprintf(fulltype, "%s/%s", supertype, subtype);

									// Process the subtype
									ProcessType(fulltype, &bytesNeeded);
									if (bytesNeeded > maxBytesNeeded)
										maxBytesNeeded = bytesNeeded;
								}
							}
						}
					} else {
						DBG(OUT("Mime::SnifferRules::BuildRuleList(): "
						          "Failed opening supertype directory '%s'\n",
						            supertype));
					}

					// Second, process the supertype
					ProcessType(supertype, &bytesNeeded);
					if (bytesNeeded > maxBytesNeeded)
						maxBytesNeeded = bytesNeeded;
				}
			}
		}
	} else {
		DBG(OUT("Mime::SnifferRules::BuildRuleList(): "
		          "Failed opening mime database directory '%s'\n",
		            get_database_directory().c_str()));
	}

	if (!err) {
		fRuleList.sort();
		fMaxBytesNeeded = maxBytesNeeded;
		fHaveDoneFullBuild = true;
//		PrintToStream();
	} else {
		DBG(OUT("Mime::SnifferRules::BuildRuleList() failed, error code == 0x%"
			B_PRIx32 "\n", err));
	}
	return err;
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the supplied chunk of data.

	This is accomplished by searching through the currently installed
	list of sniffer rules for a rule that matches on the given data buffer.
	Rules are searched in order of priority (higher priority first). Rules
	of equal priority are searched in reverse-alphabetical order (that way
	"supertype/subtype" form rules are checked before "supertype-only" form
	rules if their priorities happen to be identical).

	\param file The file to sniff. May be \c NULL. \a buffer is always given.
	\param buffer Pointer to a data buffer to sniff
	\param length The length of the data buffer pointed to by \a buffer
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success
	- \c Mime::kMimeGuessFailure: no match found (\a type is left unmodified)
	- error code: failure
*/
status_t
SnifferRules::GuessMimeType(BFile* file, const void *buffer, int32 length,
	BString *type)
{
	status_t err = buffer && type ? B_OK : B_BAD_VALUE;
	if (err)
		return err;

	// wrap the buffer by a BMemoryIO
	BMemoryIO data(buffer, length);

	if (!err && !fHaveDoneFullBuild)
		err = BuildRuleList();

	// first ask the MimeSnifferAddonManager for a suitable type
	float addonPriority = -1;
	BMimeType mimeType;
	if (!err) {
		MimeSnifferAddonManager* manager = MimeSnifferAddonManager::Default();
		if (manager) {
			addonPriority = manager->GuessMimeType(file, buffer, length,
				&mimeType);
		}
	}

	if (!err) {
		// Run through our rule list, which is sorted in order of
		// descreasing priority, and see if one of the rules sniffs
		// out a match
		for (std::list<sniffer_rule>::const_iterator i = fRuleList.begin();
			   i != fRuleList.end();
			     i++)
		{
			if (i->rule) {
				// If an add-on identified the type with a priority at least
				// as great as the remaining rules, we can stop further
				// processing and return the type found by the add-on.
				if (i->rule->Priority() <= addonPriority) {
					*type = mimeType.Type();
					return B_OK;
				}

				if (i->rule->Sniff(&data)) {
					type->SetTo(i->type.c_str());
					return B_OK;
				}
			} else {
				DBG(OUT("WARNING: Mime::SnifferRules::GuessMimeType(BPositionIO*,BString*): "
					"NULL sniffer_rule::rule member found in rule list for type == '%s', "
					"rule_string == '%s'\n",
					i->type.c_str(), i->rule_string.c_str()));
			}
		}

		// The sniffer add-on manager might have returned a low priority
		// (lower than any of a rule).
		if (addonPriority >= 0) {
			*type = mimeType.Type();
			return B_OK;
		}

		// If we get here, we didn't find a damn thing
		err = kMimeGuessFailureError;
	}
	return err;
}

// MaxBytesNeeded
/*! \brief Returns the maxmimum number of bytes needed in a data buffer for
	all the currently installed rules to be able to perform a complete sniff,
	or an error code if something goes wrong.

	If the internal rule list has not yet been built (this includes parsing
	all the installed rules), it will be.

	\return: If the return value is non-negative, it represents	the max number
	of bytes needed to do a complete sniff. Otherwise, the number returned is
	an error code.
*/
ssize_t
SnifferRules::MaxBytesNeeded()
{
	ssize_t err = fHaveDoneFullBuild ? B_OK : BuildRuleList();
	if (!err) {
		err = fMaxBytesNeeded;
		MimeSnifferAddonManager* manager = MimeSnifferAddonManager::Default();
		if (manager) {
			fMaxBytesNeeded = max_c(fMaxBytesNeeded,
				(ssize_t)manager->MinimalBufferSize());
		}
	}
	return err;
}

// ProcessType
/*! \brief Handles a portion of the initial rule list construction for
	the given mime type.

	\note To be called by BuildRuleList() *ONLY*. :-)

	\param type The mime type of interest. The mime string is expected to be valid
	            and lowercase. Both "supertype" and "supertype/subtype" mime types
	            are allowed.
	\param bytesNeeded Returns the minimum number of bytes needed for this rule to
	                   perform a complete sniff. May not be NULL because I'm lazy
	                   and this function is for internal use only anyway.
	\return
	The return value is essentially ignored (as this function prints out the
	debug warning if a parse fails), but that being said:
	- \c B_OK: success
	- \c other error code: failure
*/
status_t
SnifferRules::ProcessType(const char *type, ssize_t *bytesNeeded)
{
	status_t err = type && bytesNeeded ? B_OK : B_BAD_VALUE;
	if (!err)
		*bytesNeeded = 0;

	BString str;
	BString errorMsg;
	sniffer_rule rule(new Sniffer::Rule());

	// Check the mem alloc
	if (!err)
		err = rule.rule ? B_OK : B_NO_MEMORY;
	// Read the attr
	if (!err)
		err = read_mime_attr_string(type, kSnifferRuleAttr, &str);
	// Parse the rule
	if (!err) {
		err = Sniffer::parse(str.String(), rule.rule, &errorMsg);
		if (err)
			DBG(OUT("WARNING: SnifferRules::ProcessType(): Parse failure:\n%s\n", errorMsg.String()));
	}
	if (!err) {
		// Note the bytes needed
		*bytesNeeded = rule.rule->BytesNeeded();

		// Add the rule to the list
		rule.type = type;
		rule.rule_string = str.String();
		fRuleList.push_back(rule);
	}
	return err;
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

