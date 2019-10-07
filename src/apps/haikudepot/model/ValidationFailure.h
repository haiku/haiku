/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef VALIDATION_FAILURE_H
#define VALIDATION_FAILURE_H


#include <Archivable.h>
#include <ObjectList.h>
#include <String.h>
#include <StringList.h>


/*!	This objects marries together a 'key' into some data together with an
	error message.  The error message is not intended to be human readable, but
	is a key into a message.
*/

class ValidationFailure : public BArchivable {
public:
								ValidationFailure(BMessage* from);
								ValidationFailure(const BString& property);
	virtual						~ValidationFailure();

	const	BString&			Property() const;
	const	BStringList&		Messages() const;
			bool				IsEmpty() const;
			bool				Contains(const BString& message) const;

			void				AddMessage(const BString& value);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fProperty;
			BStringList			fMessages;
};


class ValidationFailures : public BArchivable {
public:
								ValidationFailures(BMessage* from);
								ValidationFailures();
	virtual						~ValidationFailures();

			void				AddFailure(const BString& property,
									const BString& message);
			int32				CountFailures() const;
			ValidationFailure*	FailureAtIndex(int32 index) const;
			bool				IsEmpty() const;
			bool				Contains(const BString& property) const;
			bool				Contains(const BString& property,
									const BString& message) const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			void				_AddFromMessage(const BMessage* from);
			ValidationFailure*	_GetFailure(const BString& property) const;
			ValidationFailure*	_GetOrCreateFailure(const BString& property);

private:
			BObjectList<ValidationFailure>
								fItems;
};


#endif // VALIDATION_FAILURE_H
