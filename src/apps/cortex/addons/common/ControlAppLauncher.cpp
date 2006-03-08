// ControlAppLauncher.cpp
// e.moon 17jun99

#include "ControlAppLauncher.h"

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ControlAppLauncher::~ControlAppLauncher() {
	if(launched())
		quit();
}
	
ControlAppLauncher::ControlAppLauncher(
	media_node_id mediaNode,
	bool launchNow) :

	m_mediaNode(mediaNode),
	m_error(B_OK) {
	
	if(launchNow)
		m_error = launch();
}
				
// -------------------------------------------------------- //
// operations
// -------------------------------------------------------- //

status_t ControlAppLauncher::launch(); //nyi
	
// -------------------------------------------------------- //
// guts
// -------------------------------------------------------- //

void ControlAppLauncher::quit();


// END -- ControlAppLauncher.cpp --