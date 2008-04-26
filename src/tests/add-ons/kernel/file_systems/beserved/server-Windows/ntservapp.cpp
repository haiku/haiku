// NTService.cpp
// 
// This is the main program file containing the entry point.

#include "stdafx.h"
#include "winsvc.h"

#include "NTServApp.h"
#include "myservice.h"

int main(int argc, char *argv[])
{
	// Create the service object
	CMyService MyService;

	// Parse for standard arguments (install, uninstall, version etc.)
	if (!MyService.ParseStandardArgs(argc, argv))
		MyService.StartService();

	// When we get here, the service has been stopped
	return MyService.m_Status.dwWin32ExitCode;
}
