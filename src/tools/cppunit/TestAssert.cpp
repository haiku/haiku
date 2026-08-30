/*
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <cppunit/Portability.h>
#include <cppunit/TestAssert.h>
#include <TestUtils.h>

std::string DecodeResult(status_t result) {
	switch (result) {
		case B_OK: return "B_OK";
		case B_ERROR: return "B_ERROR";
		case B_FILE_ERROR: return "B_FILE_ERROR";
		case B_FILE_EXISTS: return "B_FILE_EXISTS";
		case B_ENTRY_NOT_FOUND: return "B_ENTRY_NOT_FOUND";
		case B_NAME_TOO_LONG: return "B_NAME_TOO_LONG";
		case B_DIRECTORY_NOT_EMPTY: return "B_DIRECTORY_NOT_EMPTY";
		case B_DEVICE_FULL: return "B_DEVICE_FULL";
		case B_READ_ONLY_DEVICE: return "B_READ_ONLY_DEVICE";
		case B_IS_A_DIRECTORY: return "B_IS_A_DIRECTORY";
		case B_NO_MORE_FDS: return "B_NO_MORE_FDS";
		case B_CROSS_DEVICE_LINK: return "B_CROSS_DEVICE_LINK";
		case B_LINK_LIMIT: return "B_LINK_LIMIT";
		case B_BUSTED_PIPE: return "B_BUSTED_PIPE";
		case B_UNSUPPORTED: return "B_UNSUPPORTED";
		case B_PARTITION_TOO_SMALL: return "B_PARTITION_TOO_SMALL";
		case B_PARTIAL_READ: return "B_PARTIAL_READ";
		case B_PARTIAL_WRITE: return "B_PARTIAL_WRITE";
		case B_BAD_MIME_SNIFFER_RULE: return "B_BAD_MIME_SNIFFER_RULE";
		case B_NO_MEMORY: return "B_NO_MEMORY";
		case B_IO_ERROR: return "B_IO_ERROR";
		case B_PERMISSION_DENIED: return "B_PERMISSION_DENIED";
		case B_BAD_INDEX: return "B_BAD_INDEX";
		case B_BAD_TYPE: return "B_BAD_TYPE";
		case B_BAD_VALUE: return "B_BAD_VALUE";
		case B_MISMATCHED_VALUES: return "B_MISMATCHED_VALUES";
		case B_NAME_NOT_FOUND: return "B_NAME_NOT_FOUND";
		case B_NAME_IN_USE: return "B_NAME_IN_USE";
		case B_TIMED_OUT: return "B_TIMED_OUT";
		case B_INTERRUPTED: return "B_INTERRUPTED";
		case B_WOULD_BLOCK: return "B_WOULD_BLOCK";
		case B_CANCELED: return "B_CANCELED";
		case B_NO_INIT: return "B_NO_INIT";
		case B_BUSY: return "B_BUSY";
		case B_NOT_ALLOWED: return "B_NOT_ALLOWED";
		case B_BAD_ADDRESS: return "B_BAD_ADDRESS";
		case B_BAD_TEAM_ID: return "B_BAD_TEAM_ID";
		case B_BAD_PORT_ID: return "B_BAD_PORT_ID";
		default: return "??????????";
	}
}

namespace CppUnit {

template<>
struct assertion_traits<status_t> {
	static bool equal( const status_t& x, const status_t& y )
	{
		return x == y;
	}

	static std::string toString( const status_t& x )
	{
		return DecodeResult(x);
	}
};

}
