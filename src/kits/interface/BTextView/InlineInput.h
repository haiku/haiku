#ifndef __INLINEINPUT_H
#define __INLINEINPUT_H

#include <Messenger.h>

class _BInlineInput_ {
public:
	_BInlineInput_(BMessenger);
	~_BInlineInput_();
	
	const BMessenger *Method() const;
	
	bool IsActive() const;
	void SetActive(bool active);
	
	int32 Length() const;
	void SetLength(int32 length);
	
	int32 Offset() const;
	void SetOffset(int32 offset);
	
private:
	const BMessenger fMessenger;

	bool fActive;
		
	int32 fOffset;
	int32 fLength;
};

#endif //__INLINEINPUT_H
