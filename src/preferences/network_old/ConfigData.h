#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <String.h>
#include <File.h>
#include <List.h>

class InterfaceData
{
public:
	InterfaceData(const char *name);
	
	BString fConfig;
	BString fPath;
	BString fType;
	BString	fIPAddress;
	BString fNetMask;
	BString fGateway;
	BString fPrettyName;
	BString fName;
	
	bool fEnabled;
	bool fDHCP;
};

class ConfigData
{
public:
	ConfigData(void);
	~ConfigData(void);
	void EmptyInterfaces(void);
	
	BString fDomainName;
	BString	fHostname;
	BString fPrimaryDNS;
	BString fSecondaryDNS;
	
	bool	fFTPServer;
	bool	fTelnetServer;
	
	BString	fUsername;
	BString	fPassword;
	BString fProtocols;
	
	bool fAppleTalk;
	bool fIPForwarding;
	bool fUseDNS;
	
	BList fInterfaceList;
};

void SaveBackup(const char *filename, ConfigData &data);
status_t LoadBackup(const char *pathname, ConfigData &data);

#endif
