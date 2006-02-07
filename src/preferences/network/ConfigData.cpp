#include "ConfigData.h"
#include <stdio.h>

InterfaceData::InterfaceData(const char *name)
 :
 	fConfig(""),
 	fPath(""),
 	fType("ETHERNET"),
 	fIPAddress("192.168.0.2"),
 	fNetMask("255.255.255.0"),
 	fGateway("192.168.0.1"),
 	fPrettyName(""),
 	fName(name),
  	fEnabled(true),
 	fDHCP(false)
{
}

ConfigData::ConfigData(void)
 :	fDomainName(""),
 	fHostname(""),
 	fPrimaryDNS(""),
 	fSecondaryDNS(""),
 	fFTPServer(false),
 	fTelnetServer(false),
 	fUsername(""),
 	fPassword(""),
 	fProtocols(""),
 	fAppleTalk(false),
 	fIPForwarding(false),
 	fUseDNS(true),
 	fInterfaceList(0)
{
}

ConfigData::~ConfigData(void)
{
	EmptyInterfaces();
}

void
ConfigData::EmptyInterfaces(void)
{
	for(int32 i=0; i<fInterfaceList.CountItems(); i++)
	{
		InterfaceData *data = (InterfaceData*) fInterfaceList.ItemAt(i);
		delete data;
	}
}

void
SaveBackup(const char *filename, ConfigData &data)
{
	BString filedata("GLOBAL:");
	
	filedata << "\n\tDNS_DOMAIN = " << data.fDomainName;
	filedata << "\n\tHOSTNAME = " << data.fHostname;
	filedata << "\n\tUSERNAME = " << data.fUsername;
	
	// This is currently unused. Vestigial limb, perhaps?
	filedata << "\n\tPROTOCOLS = " << data.fProtocols;
	filedata << "\n\tPASSWORD = " << data.fPassword;
	filedata << "\n\tFTP_ENABLED = " << (data.fFTPServer ? "1" : "0");
	filedata << "\n\tIP_FORWARD = " << (data.fIPForwarding ? "1" : "0");
	filedata << "\n\tDNS_ENABLED = " << (data.fUseDNS ? "1" : "0");
	filedata << "\n\tTELNETD_ENABLED = " << (data.fTelnetServer ? "1" : "0");
	filedata << "\n\tDNS_PRIMARY = " << data.fPrimaryDNS;
	filedata << "\n\tDNS_SECONDARY = " << data.fSecondaryDNS;
	
	// Is this another vestigial limb from an older OS version?
	filedata << "\n\tROUTER = " << "192.168.0.1";
	
	// I'm assuming that this is the network settings file format
	filedata << "\n\tVERSION = " << "V1.00";
	
	// We'll add the trailing space when we add the interface names
	filedata << "\n\tINTERFACES =";
	
	// Now let's see how many interfaces there are
	int32 count = data.fInterfaceList.CountItems();
	
	// special hack if there are no available network interfaces. :)
	if(count==0)
		filedata << " ";
	
	for(int32 i=0; i<count; i++)
		filedata << " interface" << i;
	
	// Now onto the interface sections
	
	for(int32 i=0; i<count; i++) {
	
		InterfaceData *interface = (InterfaceData*) data.fInterfaceList.ItemAt(i);
		if(!interface)
			continue;
		
		filedata << "\ninterface" << i << ":";
		
		filedata << "\n\tDEVICECONFIG = " << interface->fConfig;
		filedata << "\n\tDEVICELINK = " << interface->fPath;
		filedata << "\n\tDEVICETYPE = " << interface->fType;
		filedata << "\n\tIPADDRESS = " << interface->fIPAddress;
		filedata << "\n\tNETMASK = " << interface->fNetMask;
		filedata << "\n\tPRETTYNAME = " << interface->fPrettyName;
		filedata << "\n\tENABLED = " << (interface->fEnabled ? "1" : "0");
		filedata << "\n\tDHCP = " << (interface->fDHCP ? "1" : "0");
	}
	
	filedata << "\n";
	
	// Now let's write all this stuff
	BString path("/boot/home/config/settings/network");
	
	// This SaveBackup() function is used for backups(so a network.<name> file)
	// and for the "Save" button which uses the network file as the backup file. 
	if(filename)             
		path << "." << filename;
	
	BFile file(path.String(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if(file.InitCheck()!=B_OK)
	{
		printf("DEBUG: Couldn't create network settings file %s\n",path.String());
		return;
	}
	
	file.Write(filedata.String(),filedata.Length());
	
	file.Unset();
}

status_t
LoadBackup(const char *pathname, ConfigData &data)
{
	// The configuration is saved in the /boot/home/config/settings/network
	// text file so the config is static. Maybe we could switch to dynamic by
	// directly asking the net_server how things are
	
	// This function being used when pressing "Revert" so first we need to
	// empty things before setting the new values
	ConfigData empty;
	data.EmptyInterfaces();
	data = empty;
	
	// Read in the data from the file
			
	BString pathstring = "/boot/home/config/settings/network";
	if(pathname)
		pathstring << "." << pathname;
	
	BFile file(pathstring.String(),B_READ_ONLY);
	if(file.InitCheck()!=B_OK)
		return file.InitCheck();
	
	off_t filesize;
	file.GetSize(&filesize);
	
	if(filesize < 1)
		return B_ERROR;
	
	char *filestr = new char[filesize + 1];
	
	filestr[filesize + 1] = 0;
	file.Read(filestr, filesize);
	file.Unset();

	char *start=NULL, *end=NULL;
	
	// This is a little bit more flexible (and slower) implementation than may be necessary. We do use a small
	// string hack to avoid unnecessary copying
	start = strstr(filestr,"HOSTNAME = ");
	if(start) {
	
		end = strstr(start,"\n");
		if(end) {
		
			*end='\0';
			data.fHostname = start;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"USERNAME = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fUsername = start + 11;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"HOSTNAME = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fHostname = start + 11;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"PROTOCOLS = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fProtocols = start + 12;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"PASSWORD = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fPassword = start + 11;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"FTP_ENABLED = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			start += 14;
			data.fFTPServer = (strcmp(start,"1")==0)?true:false;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"IP_FORWARD = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			start += 13;
			data.fIPForwarding = (strcmp(start,"1")==0)?true:false;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"DNS_ENABLED = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			start += 14;
			data.fUseDNS = (strcmp(start,"1")==0)?true:false;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"TELNETD_ENABLED = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			start += 18;
			data.fTelnetServer = (strcmp(start,"1")==0)?true:false;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"DNS_DOMAIN = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fDomainName = start + 13;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"DNS_PRIMARY = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fPrimaryDNS = start + 14;
			*end='\n';
		}
	}
	
	start = strstr(filestr,"DNS_SECONDARY = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fSecondaryDNS = start + 16;
			*end='\n';
		}
	}
	
	// TODO: What should we do with the ROUTER setting if there are more then one card and both
	// specify settings?

/*
	start = strstr(filestr,"ROUTER = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fGateway = start + 9;
			*end='\n';
		}
	}

	start = strstr(filestr,"VERSION = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			data.fVersion = start + 9;
			*end='\n';
		}
	}
*/
	
	start = strstr(filestr,"INTERFACES = ");
	if(start) {
	
		end = strchr(start,'\n');
		if(end) {
		
			*end='\0';
			BString interfaces = start + 13;
			*end='\n';
			
			// Now that we have them, lets find the names of all the interfaces present
			int32 startpos=0,endpos=0;
			while(endpos>=0)
			{
				endpos = interfaces.FindFirst(" ",startpos);
				
				BString name = interfaces.String() + startpos;
				
				if(endpos>=0)
					name.Truncate(endpos);
				
				data.fInterfaceList.AddItem(new InterfaceData(name.String()));
				
				startpos = endpos+1;
			}
		}
	}
	
	// OK. Now we've gotten through the GLOBAL section. All that's left is to read each interface section
	for(int32 i=0; i<data.fInterfaceList.CountItems(); i++)
	{
		InterfaceData *idata = (InterfaceData*)data.fInterfaceList.ItemAt(i);
		
		BString searchstr(idata->fName.String());
		searchstr << ":";
		
		char *istart = strstr(filestr,searchstr.String());
		if(!istart)
			continue;
		
		istart = strchr(istart,'\n') + 1;
		
		BString istring = istart;
		int32 istringend = istring.FindFirst("interface");
		if(istringend>=0)
			istring.Truncate(istringend);
		
		// OK. At this point, we should have in istring all the data for the interface
		int32 datastart=0,dataend=0;
		datastart = istring.FindFirst("DEVICECONFIG = ");
		if(datastart>=0)
		{
			idata->fConfig = istring.String() + datastart;
			dataend = idata->fConfig.FindFirst("\n");
			if(dataend>=0)
				idata->fConfig.Truncate(dataend);
			
			idata->fConfig = BString(idata->fConfig.String() + 15);
		}

		datastart = istring.FindFirst("DEVICELINK = ");
		if(datastart>=0)
		{
			idata->fPath = istring.String() + datastart;
			dataend = idata->fPath.FindFirst("\n");
			if(dataend>=0)
				idata->fPath.Truncate(dataend);
			
			idata->fPath = BString(idata->fPath.String() + 13);
		}
		
		datastart = istring.FindFirst("DEVICETYPE = ");
		if(datastart>=0)
		{
			idata->fType = istring.String() + datastart;
			dataend = idata->fType.FindFirst("\n");
			if(dataend>=0)
				idata->fType.Truncate(dataend);
			
			idata->fType = BString(idata->fType.String() + 13);
		}
		
		datastart = istring.FindFirst("IPADDRESS = ");
		if(datastart>=0)
		{
			idata->fIPAddress = istring.String() + datastart;
			dataend = idata->fIPAddress.FindFirst("\n");
			if(dataend>=0)
				idata->fIPAddress.Truncate(dataend);
			
			idata->fIPAddress = BString(idata->fIPAddress.String() + 12);
		}
		
		datastart = istring.FindFirst("NETMASK = ");
		if(datastart>=0)
		{
			idata->fNetMask = istring.String() + datastart;
			dataend = idata->fNetMask.FindFirst("\n");
			if(dataend>=0)
				idata->fNetMask.Truncate(dataend);
			
			idata->fNetMask = BString(idata->fNetMask.String() + 10);
		}
		
		datastart = istring.FindFirst("PRETTYNAME = ");
		if(datastart>=0)
		{
			idata->fPrettyName = istring.String() + datastart;
			dataend = idata->fPrettyName.FindFirst("\n");
			if(dataend>=0)
				idata->fPrettyName.Truncate(dataend);
			
			idata->fPrettyName = BString(idata->fPrettyName.String() + 13);
		}
		
		datastart = istring.FindFirst("ENABLED = ");
		if(datastart>=0)
		{
			BString str = istring.String() + datastart;
			dataend = str.FindFirst("\n");
			if(dataend>=0)
				str.Truncate(dataend);
			
			idata->fEnabled = (strcmp(str.String() + 10,"1")==0);
		}
		
		datastart = istring.FindFirst("DHCP = ");
		if(datastart>=0)
		{
			BString str = istring.String() + datastart;
			dataend = str.FindFirst("\n");
			if(dataend>=0)
				str.Truncate(dataend);
			
			idata->fDHCP = (strcmp(str.String() + 7,"1")==0);
		}
		
	}
	
	
	return B_OK;
}
