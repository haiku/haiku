/*
	ToneProducerApp main.cpp

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "NodeHarnessApp.h"
#include "MediaNodeControlApp.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <MediaDefs.h>

const char* const		g_pAppSignature = "application/x-vnd.Be.ToneProducerApp";

int main(int argc, char** argv) {
	if(argc < 2 || strstr(argv[1], "node=") != argv[1]) {
		// start in stand-alone app mode
		NodeHarnessApp app(g_pAppSignature);
		app.Run();
	}
	else {
		// start control-panel app
		media_node_id id = atol(argv[1] + 5);
		MediaNodeControlApp app(g_pAppSignature, id);
		app.Run();
	}
	
	return 0;
}
