#include <module.h>

struct audio_module_info
{ 
	module_info module;
	void (*print_hello)();
};

typedef struct audio_module_info audio_module_info;
