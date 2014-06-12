/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "BitFieldTesters.h"

#include <stdio.h>


#define NOTE "NotFieldTester : "
#define MINI "MinMatchFieldTester : "


BitFieldTester::BitFieldTester()
{
}


BitFieldTester::BitFieldTester(BMessage* from)
	:
	BArchivable(from)
{
}


status_t
BitFieldTester::Archive(BMessage* into, bool deep) const
{
	return BArchivable::Archive(into, deep);
}


//	#pragma mark - ConstantFieldTester


ConstantFieldTester::ConstantFieldTester(bool result)
	:
	fResult(result)
{
}


ConstantFieldTester::ConstantFieldTester(BMessage* from)
	:
	BitFieldTester(from)
{
	if (from->FindBool("ctRes", &fResult) != B_OK)
		printf("ConstantFieldTester: Error, no ctRes!\n");
}


status_t
ConstantFieldTester::Archive(BMessage* into, bool deep) const
{
	status_t result = BitFieldTester::Archive(into, deep);
	into->AddBool("ctRes", fResult);
	return result;
}


BArchivable*
ConstantFieldTester::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "ConstantFieldTester"))
		return new ConstantFieldTester(from);
	else
		return NULL;
}


bool
ConstantFieldTester::IsMatching(uint32 field)
{
	return fResult;
}


//	#pragma mark - HasBitsFieldTester


HasBitsFieldTester::HasBitsFieldTester(uint32 requiredBits,
	uint32 forbiddenBits)
	:
	fRequiredBits(requiredBits),
	fForbiddenBits(forbiddenBits)
{
}


HasBitsFieldTester::HasBitsFieldTester(BMessage* from)
	:
	BitFieldTester(from)
{
	if (from->FindInt32("rqBits", (int32*) &fRequiredBits) != B_OK)
		printf("HasBitsFieldTester: Error, no rqBits!\n");

	if (from->FindInt32("fbBits", (int32*) &fForbiddenBits) != B_OK)
		printf("HasBitsFieldTester: Error, no fbBits!\n");
}


status_t
HasBitsFieldTester::Archive(BMessage* into, bool deep) const
{
	status_t result = BitFieldTester::Archive(into, deep);
	into->AddInt32("rqBits", fRequiredBits);
	into->AddInt32("fbBits", fForbiddenBits);

	return result;
}


BArchivable*
HasBitsFieldTester::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "HasBitsFieldTester"))
		return new HasBitsFieldTester(from);
	else
		return NULL;
}


bool
HasBitsFieldTester::IsMatching(uint32 field)
{
	return ((((fRequiredBits & (~field)) == 0))
		&& ((fForbiddenBits & (~field)) == fForbiddenBits));
}


//	#pragma mark - NotFieldTester


NotFieldTester::NotFieldTester(BitFieldTester* slave)
	:
	fSlave(slave)
{
}


NotFieldTester::~NotFieldTester()
{
	delete fSlave;
}


NotFieldTester::NotFieldTester(BMessage* from)
	:
	BitFieldTester(from),
	fSlave(NULL)
{
	BMessage slaveMessage;
	if (from->FindMessage("nSlave", &slaveMessage) == B_OK) {
		BArchivable* slaveObject = instantiate_object(&slaveMessage);
		if (slaveObject != NULL) {
			fSlave = dynamic_cast<BitFieldTester*>(slaveObject);
			if (fSlave == NULL) {
				printf(NOTE "Error casting slaveObject to BitFieldTester!\n");
				delete slaveObject;
			}
		} else
			printf(NOTE "instantiate_object returned NULL!\n");
	} else
		printf(NOTE "Couldn't unarchive NotFieldTester slave!\n");
}


status_t
NotFieldTester::Archive(BMessage* into, bool deep) const
{
	if (fSlave == NULL)
		return B_ERROR;

	status_t result = BitFieldTester::Archive(into, deep);
	if (result == B_OK) {
		BMessage message;
		result = fSlave->Archive(&message, deep);
		into->AddMessage("nSlave", &message);
	}

	return result;
}


BArchivable*
NotFieldTester::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "NotFieldTester"))
		return new NotFieldTester(from);
	else
		return NULL;
}


bool
NotFieldTester::IsMatching(uint32 field)
{
	return fSlave ? (!fSlave->IsMatching(field)) : false;
}


//	#pragma mark - MinMatchFieldTester


MinMatchFieldTester::MinMatchFieldTester(int32 minNum, bool deleteSlaves)
	:
	fMinNum(minNum),
	fDeleteSlaves(deleteSlaves) // fDeleteSlaves state not archived!
{
}


MinMatchFieldTester::~MinMatchFieldTester()
{
	if (fDeleteSlaves) {
		int32 last = fSlaves.CountItems();
		for (int32 i = 0; i < last; i++)
			delete ((BitFieldTester*) fSlaves.ItemAt(i));
	}
}


MinMatchFieldTester::MinMatchFieldTester(BMessage* from)
	:
	BitFieldTester(from),
	fDeleteSlaves(true)
{
	int32 i = 0;
	BMessage slaveMessage;
	while (from->FindMessage("mSlave", i++, &slaveMessage) == B_OK) {
		BArchivable* slaveObject = instantiate_object(&slaveMessage);
		if (slaveObject) {
			BitFieldTester* nextSlave = dynamic_cast<BitFieldTester*>(slaveObject);
			if (nextSlave)
				fSlaves.AddItem(nextSlave);
			else {
				printf(MINI "Error casting slaveObject to BitFieldTester!\n");
				delete slaveObject;
			}
		} else
			printf(MINI "instantiate_object returned NULL!\n");
	}

	if (from->FindInt32("mMin", (int32*)&fMinNum) != B_OK)
		puts(MINI "Error getting mMin!");
}


// (slave) should be allocated with new, becomes property of this object.
void
MinMatchFieldTester::AddSlave(const BitFieldTester* slave)
{
	fSlaves.AddItem((void*) slave);
}


status_t
MinMatchFieldTester::Archive(BMessage* into, bool deep) const
{
	status_t result = BitFieldTester::Archive(into, deep);
	if (result == B_OK) {
		int32 last = fSlaves.CountItems();
		for (int32 i = 0; i < last; i++) {
			BMessage msg;
			result = ((BitFieldTester*)fSlaves.ItemAt(i))->Archive(&msg, deep);
			if (result != B_OK)
				return result;

			into->AddMessage("mSlave", &msg);
		}
	}
	into->AddInt32("mMin", fMinNum);

	return result;
}


BArchivable*
MinMatchFieldTester::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MinMatchFieldTester"))
		return new MinMatchFieldTester(from);
	else
		return NULL;
}


// Returns true if at least (fMinNum) slaves return true.
bool
MinMatchFieldTester::IsMatching(uint32 field)
{
	int32 last = fSlaves.CountItems();
	if (fMinNum == 0 && last == 0) {
		// 0 >= 0, so this should return true
		return true;
	}

	int32 count = 0;

	for (int32 i = 0; i < last; i++) {
		if ((((BitFieldTester*)fSlaves.ItemAt(i))->IsMatching(field))
				&& (++count >= fMinNum)) {
			return true;
		}
	}

	return false;
}
