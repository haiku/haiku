#ifndef MAILAPP_H
#define MAILAPP_H

#include <app/Application.h>

class mailApp : public BApplication
{
	public:
   	mailApp(void);
   		~mailApp();

	virtual void sendMail(const char *subject, const char*body, const char *to, const char *cc, const char *bcc);
//private:

};

#endif
