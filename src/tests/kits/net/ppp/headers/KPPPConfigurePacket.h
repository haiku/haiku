#ifndef _K_PPP_CONFIGURE_PACKET__H
#define _K_PPP_CONFIGURE_PACKET__H

#include <mbuf.h>


typedef struct configure_item {
	void *data;
	int32 len;
	uint8 type;
} configure_item;


class PPPConfigurePacket {
	public:
		PPPConfigurePacket(uint8 type);
		PPPConfigurePacket(mbuf *data);
		~PPPConfigurePacket();
		
		bool SetType(uint8 type);
		uint8 Type() const
			{ return fType; }
		
		void AddItem(configure_item *item);
		bool RemoveItem(configure_item *item);
		int32 CountItems() const;
		configure_item *ItemAt(int32 index) const;
		
		mbuf *ToMbuf() const;

	private:
		uint8 fType;
};


#endif
