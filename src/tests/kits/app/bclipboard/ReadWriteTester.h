//------------------------------------------------------------------------------
//	ReadWriteTester.h
//
//------------------------------------------------------------------------------

#ifndef READ_WRITE_TESTER_H
#define READ_WRITE_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class ReadWriteTester : public TestCase
{
	public:
		ReadWriteTester() {;}
		ReadWriteTester(std::string name) : TestCase(name) {;}

		void Clear1();
		void Clear2();
		void Revert1();
		void Revert2();
		void Commit1();
		void Commit2();
		void Data1();
		void Data2();
		void DataSource1();
		void DataSource2();
		void DataSource3();
		void StartWatching1();
		void StopWatching1();
		void StopWatching2();

		static Test* Suite();
};

class RWHandler : public BHandler {
public:
	RWHandler();

	virtual void MessageReceived(BMessage *message);
	bool ClipboardModified();
private:
	bool fClipboardModified;
};

#endif	// READ_WRITE_TESTER_H

