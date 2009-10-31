/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
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
	// empty
}


BitFieldTester::BitFieldTester(BMessage* from)
	:
	BArchivable(from)
{
	// empty
}


status_t
BitFieldTester::Archive(BMessage* into, bool deep) const
{
	return BArchivable::Archive(into, deep);
} 


// ---------------- ConstantFieldTester starts -------------------------------
ConstantFieldTester::ConstantFieldTester(bool result)
	:
	fResult(result)
{
	// empty
}


ConstantFieldTester::ConstantFieldTester(BMessage* from)
	:
	BitFieldTester(from)
{
	if (from->FindBool("ctRes", &fResult) != B_NO_ERROR)
		printf("ConstantFieldTester: Error, no ctRes!\n");
}

 
status_t
ConstantFieldTester::Archive(BMessage* into, bool deep) const
{
	status_t ret = BitFieldTester::Archive(into, deep);
	into->AddBool("ctRes", fResult);
	return ret;
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


// ---------------- HasBitsFieldTester starts -------------------------------
HasBitsFieldTester::HasBitsFieldTester(uint32 requiredBits, 
	uint32 forbiddenBits)
	: 
	fRequiredBits(requiredBits), 
	fForbiddenBits(forbiddenBits)
{
	// empty
}


HasBitsFieldTester::HasBitsFieldTester(BMessage* from)
	:
	BitFieldTester(from)
{
	if (from->FindInt32("rqBits", (int32*) &fRequiredBits) != B_NO_ERROR)
		printf("HasBitsFieldTester: Error, no rqBits!\n");
	
	if (from->FindInt32("fbBits", (int32*) &fForbiddenBits) != B_NO_ERROR)
		printf("HasBitsFieldTester: Error, no fbBits!\n");
}


status_t
HasBitsFieldTester::Archive(BMessage* into, bool deep) const
{
	status_t ret = BitFieldTester::Archive(into, deep);
	into->AddInt32("rqBits", fRequiredBits);
	into->AddInt32("fbBits", fForbiddenBits);
	return ret;
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


// ---------------- NotFieldTester starts -------------------------------
NotFieldTester::NotFieldTester(BitFieldTester* slave)
	:
	fSlave(slave)
{
	// empty
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
	BMessage slaveMsg;
	if (from->FindMessage("nSlave", &slaveMsg) == B_NO_ERROR) {
		BArchivable* slaveObj = instantiate_object(&slaveMsg);
		if (slaveObj) {
			fSlave = dynamic_cast<BitFieldTester*>(slaveObj);
			if (fSlave == NULL) {
				printf(NOTE "Error casting slaveObj to BitFieldTester!\n");
				delete slaveObj;
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

	status_t ret = BitFieldTester::Archive(into, deep);

	if (ret == B_NO_ERROR) {
		BMessage msg;
		ret = fSlave->Archive(&msg, deep);
		into->AddMessage("nSlave", &msg);
	}

	return ret;
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


// ---------------- MinMatchFieldTester starts -------------------------------
MinMatchFieldTester::MinMatchFieldTester(int minNum, bool deleteSlaves)
	:
	fMinNum(minNum),
	fDeleteSlaves(deleteSlaves) // fDeleteSlaves state not archived!
{
	// empty
}


MinMatchFieldTester::~MinMatchFieldTester()
{
	if (fDeleteSlaves) {
		int nr = fSlaves.CountItems();
		for (int i = 0; i < nr; i++)
			delete ((BitFieldTester*) fSlaves.ItemAt(i));
	}
}


MinMatchFieldTester::MinMatchFieldTester(BMessage* from)
	: 
	BitFieldTester(from),
	fDeleteSlaves(true)
{
	int i = 0;
	BMessage slaveMsg;
	while (from->FindMessage("mSlave", i++, &slaveMsg) == B_NO_ERROR) {
		BArchivable* slaveObj = instantiate_object(&slaveMsg);
		if (slaveObj) {
			BitFieldTester* nextSlave = dynamic_cast<BitFieldTester*>(slaveObj);
			if (nextSlave)
				fSlaves.AddItem(nextSlave);
			else {
				printf(MINI "Error casting slaveObj to BitFieldTester!\n");
				delete slaveObj;
			}
		} else
			printf(MINI "instantiate_object returned NULL!\n");
	}

	if (from->FindInt32("mMin", (int32*) &fMinNum) != B_NO_ERROR)
		printf(MINI "Error getting mMin!\n");
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
	status_t ret = BitFieldTester::Archive(into, deep);

	if (ret == B_NO_ERROR) {
		int nr = fSlaves.CountItems();
		for (int i = 0; i < nr; i++) {
			BMessage msg;
			ret = ((BitFieldTester*)fSlaves.ItemAt(i))->Archive(&msg, deep);
			if (ret != B_NO_ERROR)
				return ret;

			into->AddMessage("mSlave", &msg);
		}
	}

	into->AddInt32("mMin", fMinNum);
	return ret;
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
	int nr = fSlaves.CountItems();
	if ((fMinNum == 0) && (nr == 0))
		return true; // 0 >= 0, so this should return true!

	int count = 0;
	
	for (int i = 0; i < nr; i++)
		if ((((BitFieldTester*)fSlaves.ItemAt(i))->IsMatching(field)) 
				&& (++count >= fMinNum))
			return true;
	return false;
}
