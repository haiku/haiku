#include <iostream.h>
#include <String.h>
#include "DebugTools.h"

void PrintStatusToStream(status_t value)
{
	// Function which simply translates a returned status code into a string
	// and dumps it to stdout
	BString outstr;
	switch(value)
	{
		case B_OK:
			outstr="B_OK ";
			break;
		case B_NAME_NOT_FOUND:
			outstr="B_NAME_NOT_FOUND ";
			break;
		case B_BAD_VALUE:
			outstr="B_BAD_VALUE ";
			break;
		case B_ERROR:
			outstr="B_ERROR ";
			break;
		case B_TIMED_OUT:
			outstr="B_TIMED_OUT ";
			break;
		case B_NO_MORE_PORTS:
			outstr="B_NO_MORE_PORTS ";
			break;
		case B_WOULD_BLOCK:
			outstr="B_WOULD_BLOCK ";
			break;
		case B_BAD_PORT_ID:
			outstr="B_BAD_PORT_ID ";
			break;
		case B_BAD_TEAM_ID:
			outstr="B_BAD_TEAM_ID ";
			break;
		default:
			outstr="undefined status value in debugtools::PrintStatusToStream() ";
			break;
	}
	cout << "Status: " << outstr.String();
}
