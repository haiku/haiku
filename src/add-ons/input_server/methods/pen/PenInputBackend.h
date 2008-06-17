#ifndef _PEN_INPUT_BACKEND_H
#define _PEN_INPUT_BACKEND_H

#include <Handler.h>


class PenInputBackend : public BHandler {
protected:
	PenInputBackend(const char *name);
	virtual ~PenInputBackend();
	
public:
	status_t InitCheck() const { return fInitStatus; };
	
private:
	status_t fInitStatus;
};

#endif /* _PEN_INPUT_BACKEND_H */
