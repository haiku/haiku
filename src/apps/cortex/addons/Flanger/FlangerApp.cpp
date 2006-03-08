// FlangerApp.cpp
// e.moon 16jun99

#include "MediaNodeControlApp.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

const char* const 	g_pAppSignature = "application/x-vnd.moon-Flanger.media_addon";

int main(int argc, char** argv) {
	if(argc < 2 || strstr(argv[1], "node=") != argv[1]) {
		fprintf(stderr, "Sorry, not a standalone application.\n");
		return -1;
	}

	// start node control panel
	media_node_id id = atol(argv[1] + 5);
	MediaNodeControlApp app(g_pAppSignature, id);
	app.Run();
		
	return 0;
}

// END -- FlangerApp.cpp --