// LoggingConsumerApp.cpp
//
// HISTORY
//   eamoon@meadgroup.com		11june99		
//   [origin: Be Developer Newsletter III.18: 5may99]

#include "NodeHarnessApp.h"
#include "MediaNodeControlApp.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

const char* const 	g_pAppSignature = "application/x-vnd.Be.LoggingConsumerApp";

int main(int argc, char** argv) {
	if(argc < 2 || strstr(argv[1], "node=") != argv[1]) {
		// start in standalone app mode
		NodeHarnessApp app(g_pAppSignature);
		app.Run();
	}
	else {
		// start node control panel
		media_node_id id = atol(argv[1] + 5);
		MediaNodeControlApp app(g_pAppSignature, id);
		app.Run();
	}
	return 0;
}

// END -- LoggingConsumerApp.cpp --