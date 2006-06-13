#include <stdio.h>
#include <Application.h>
#include <Looper.h>

class MyLooper :  public BLooper
{
public:
	MyLooper(BLooper *looper) : BLooper("test") {
		printf("Looper created\n");
		fLooper = looper;
	};

	virtual void MessageReceived(BMessage *msg) {
	printf("MessageReceived : %.4s\n", (char*)&msg->what);
	switch (msg->what) {
	case 'toto':
		if (fLooper) {
			BMessenger(fLooper).SendMessage(msg);
			break;
		}
		msg->SendReply('couc');
		break;
	default:
		BLooper::MessageReceived(msg);
	}
	};

	BLooper *fLooper;
};

class App : public BApplication
{
public:
	App() : BApplication("application/test") { 
		
	};
	virtual void ReadyToRun() {
		MyLooper looper2(NULL);
		looper2.Run();
		MyLooper looper1(&looper2);
		looper1.Run();
		printf("loopers run\n");
		BMessage reply;
		BMessenger(&looper1).SendMessage('toto', &reply);
		printf("message sent and replied\ncheck there is only a 'couc' what in  the reply\n");
		reply.PrintToStream();
		exit(0);
	};

};

int main()
{
	App().Run();
}
