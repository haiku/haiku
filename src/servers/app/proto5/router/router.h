#ifndef _ROUTER_H
#define _ROUTER_H

#include <InputServerFilter.h>
#include <List.h>
#include <OS.h>
#include <SupportDefs.h>

class PortLink;

// export this for the input_server
extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class RouterInputFilter : public BInputServerFilter {
   public:
      RouterInputFilter();
      virtual ~RouterInputFilter();
      virtual status_t InitCheck();
      virtual filter_result Filter(BMessage *message, BList *outList);
private:
   PortLink *serverlink;
};
#endif