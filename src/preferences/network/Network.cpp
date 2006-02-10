#include <Alert.h>
#include <image.h>
#include <Font.h>
#include <Messenger.h>
#include <OS.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>

#include "Network.h"
#include "NetworkWindow.h"

class Network : public BApplication
{
	public:
		Network();
		
		NetworkWindow *fwindow;
}; 

Network::Network() : BApplication("application/x-vnd.Haiku-Network")
{
	fwindow = new NetworkWindow();
	fwindow ->Show();
}

int main()
{
	Network app;
	app.Run();
	return B_NO_ERROR;
}
