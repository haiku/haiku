/* 
** Copyright 2004, Michael Pfeiffer (laplace@users.sourceforge.net).
** Distributed under the terms of the MIT License.
**
*/

#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>

class AssertStatistics {
private:
	AssertStatistics();
	
public:
	static AssertStatistics* GetInstance();
	
	void AssertFailed() { fAssertions++; fFailed++; }
	void AssertPassed() { fAssertions++; fPassed++; }

	void Print();

	int fAssertions;
	int fPassed;
	int fFailed;
	static AssertStatistics* fStatistics;
};

class Item
{
public:	
	Item() { Init(); }
	Item(const Item& item) : fValue(item.fValue) { Init(); };
	Item(int value) : fValue(value) { Init(); };
	virtual ~Item() {  fInstances --; }

	int Value() { return fValue; }

	bool Equals(Item* item) {
		return item != NULL && fValue == item->fValue;
	}

	static int GetNumberOfInstances() { return fInstances; }

	void Print() {
		fprintf(stderr, "[%d] %d", fID, fValue); 
		// fprintf(stderr, "id: %d; value: %d", fID, fValue); 
	}
	
	static int Compare(const void* a, const void* b);

private:
	void Init() {
		fID = fNextID ++;
		fInstances ++;
	}

	int fID;     // unique id for each created Item
	int fValue;  // the value of the item

	
	static int fNextID;
	static int fInstances;
};


#endif
