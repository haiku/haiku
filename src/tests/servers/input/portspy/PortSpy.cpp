#include <stdio.h>
#include "OS.h"

//
// Sequentially search for valid ports by port number and display
// their properties.
//
int main(int argc, char* argv[])
{
	port_info info;
	
	for (int port_num = 0; port_num < 9999; port_num++)
	{
		if (B_OK == get_port_info(port_num, &info) )
		{
			// Found a valid port - display it's properties.
			//
			printf("%04u: Team %u - %s\n",
				(unsigned int)info.port,
				(unsigned int)info.team,
				info.name);
		}
	}
	
	return 0;
}
