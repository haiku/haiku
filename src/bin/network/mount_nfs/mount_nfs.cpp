#include <string.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <Application.h>
#include <Roster.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef __HAIKU__
#include <fs_volume.h>
int mount(const char *filesystem, const char *where, const char *device, ulong flags, void *parameters, size_t len)
{
	(void) len;
	return fs_mount_volume(where, device, filesystem, flags, (const char *)parameters);
}
#endif

#define BUFSZ 1024

struct mount_nfs_params
{
	unsigned int serverIP;
	char *server;
	char *_export;
	uid_t uid;
	gid_t gid;
	char *hostname;
};

void usage (const char *exename);

void usage (const char *exename)
{
	printf ("usage: %s server:export mountpoint uid gid\n",exename);
}

int main (int argc, char **argv)
{
	char buf[BUFSZ];
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	//BApplication theApp ("application/x-vnd.barecode-mount_nfs");
	
	if (argc!=5)
	{
		usage(argv[0]);
		return 1;
	}	

	struct stat st;	
	if (stat(argv[2],&st)<B_NO_ERROR)
	{
		printf ("mountpoint does not exist\n");
		return 1;
	}
	
	char *colon=strchr(argv[1],':');
	
	if (colon==NULL)
	{
		usage(argv[0]);
		return 1;
	}

	int serverLength=colon-argv[1];
	char *server=new char[serverLength+1];
	memcpy (server,argv[1],serverLength);
	server[serverLength]=0;
	
	hostent *ent;
	
	ent=gethostbyname (server);
	
	if (ent==NULL)
	{
		printf ("could not get server ip\n");
		delete[] server;
		return 1;
	}

	unsigned int serverIP=ntohl(*((unsigned int *)ent->h_addr));

	mount_nfs_params params;

	params.serverIP=serverIP;
	params.server=server;
	params._export=colon+1;

	sscanf (argv[3],"%d",&params.uid);
	sscanf (argv[4],"%d",&params.gid);

	char hostname[256];
	gethostname (hostname,256);
	
	params.hostname=hostname;
	
	sprintf(buf, "nfs:%s:%s,uid=%u,gid=%u,hostname=%s", 
		inet_ntoa(*((struct in_addr *)ent->h_addr)),
		params._export,
		params.uid,
		params.gid,
		params.hostname);
		
	int result=mount ("nfs",argv[2],NULL,0,buf,strlen(buf));
	//int result=mount ("nfs",argv[2],NULL,0,&params,sizeof(params));

	delete[] server;
	
	if (result<B_NO_ERROR)
	{
		printf ("mount failed (%s)\n",strerror(result));
		return 1;
	}
	
	return 0;
}
