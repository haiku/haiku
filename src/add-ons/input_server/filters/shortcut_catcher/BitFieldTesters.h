/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef _BIT_FIELD_TESTERS_H
#define _BIT_FIELD_TESTERS_H


#include <Archivable.h>
#include <List.h>
#include <Message.h>


// This file contains various BitTester classes, each of which defines a
// sequence of bit testing logics to do on a uint32.

// The abstract base class. Defines the interface.
_EXPORT class BitFieldTester;
class BitFieldTester : public BArchivable {
public:
								BitFieldTester();
								BitFieldTester(BMessage* from);

	virtual bool				IsMatching(uint32 field) = 0;
	virtual status_t			Archive(BMessage* into,
									bool deep = true) const;
};


// This version always returns the value specified in the constructor.
_EXPORT class ConstantFieldTester;
class ConstantFieldTester : public BitFieldTester {
public:
								ConstantFieldTester(bool result);
								ConstantFieldTester(BMessage* from);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
	virtual	bool				IsMatching(uint32 field);

private:
			bool				fResult;
};


// This version matches if all requiredBits are found in the field,
// and no forbiddenBits are found.
_EXPORT class HasBitsFieldTester;
class HasBitsFieldTester : public BitFieldTester {
public:
								HasBitsFieldTester(uint32 requiredBits,
									uint32 forbiddenBits = 0);
								HasBitsFieldTester(BMessage* from);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
	virtual	bool				IsMatching(uint32 field);

private:
			uint32				fRequiredBits;
			uint32				fForbiddenBits;
};


// This one negates the tester it holds.
_EXPORT class NotFieldTester;
class NotFieldTester : public BitFieldTester {
public:
	// (slave) should be allocated with new, becomes property of this object.
								NotFieldTester(BitFieldTester* slave);
								NotFieldTester(BMessage* from);
	virtual						~NotFieldTester();

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
	virtual	bool				IsMatching(uint32 field);

private:
			BitFieldTester*		fSlave;
};


// The most interesting class: This one returns true if at least (minNum) of
// its slaves return true. It can be used for OR (i.e. minNum==1), AND
// (i.e. minNum==numberofchildren), or anything in between!
_EXPORT class MinMatchFieldTester;
class MinMatchFieldTester : public BitFieldTester {
public:
								MinMatchFieldTester(int32 minNum,
									bool deleteSlaves = true);
									MinMatchFieldTester(BMessage* from);
	virtual						~MinMatchFieldTester();

	// (slave) should be allocated with new, becomes property of this object.
			void				AddSlave(const BitFieldTester* slave);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
	virtual	bool				IsMatching(uint32 field);

private:
			BList				fSlaves;
			int32				fMinNum;

	// true if we should delete all our slaves when we are deleted.
			bool				fDeleteSlaves;
};


#endif	// _BIT_FIELD_TESTERS_H
